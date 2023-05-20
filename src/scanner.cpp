/*
    Copyright (C) 2023  Tiernan8r
    Fork of origin simple-scan at https://gitlab.gnome.org/GNOME/simple-scan

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scanner.h"
#include <string>
#include <list>
#include <sane/sane.h>
#include <sane/saneopts.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <cmath>

/* TODO: Could indicate the start of the next page immediately after the last page is received (i.e. before the sane_cancel()) */

void NotifyScanningChanged::run(Scanner scanner)
{
    scanner.signal_scanning_changed().emit();
}

NotifyUpdateDevices::NotifyUpdateDevices(std::list<ScanDevice> dvcs)
{
    devices = dvcs;
}
void NotifyUpdateDevices::run(Scanner scanner)
{
    scanner.signal_update_devices().emit(devices);
}

NotifyRequestAuthorization::NotifyRequestAuthorization(std::string rsc)
{
    resource = rsc;
}
void NotifyRequestAuthorization::run(Scanner scanner)
{
    scanner.signal_request_authorisation().emit(resource);
}

NotifyScanFailed::NotifyScanFailed(int err_code, std::string err_string)
{
    error_code = err_code;
    error_string = err_string;
}
void NotifyScanFailed::run(Scanner scanner)
{
    scanner.signal_scan_failed().emit(error_code, error_string);
}

void NotifyDocumentDone::run(Scanner scanner)
{
    scanner.signal_document_done().emit();
}

void NotifyExpectPage::run(Scanner scanner)
{
    scanner.signal_expect_page().emit();
}

NotifyGotPageInfo::NotifyGotPageInfo(int id, ScanPageInfo inf)
{
    job_id = id;
    info = info;
}
void NotifyGotPageInfo::run(Scanner scanner)
{
    if (job_id >= scanner.first_job_id && job_id < scanner.job_id)
        scanner.signal_got_page_info().emit(info);
}

NotifyPageDone::NotifyPageDone(int id)
{
    job_id = id;
}
void NotifyPageDone::run(Scanner scanner)
{
    if (job_id >= scanner.first_job_id && job_id < scanner.job_id)
        scanner.signal_page_done().emit();
}

NotifyGotLine::NotifyGotLine(int id, ScanLine ln)
{
    job_id = id;
    line = ln;
}
void NotifyGotLine::run(Scanner scanner)
{
    if (job_id >= scanner.first_job_id && job_id < scanner.job_id)
        scanner.signal_got_line().emit(line);
}

Scanner::Scanner(void)
{
    request_queue = new AsyncQueue<Request>();
    notify_queue = new AsyncQueue<Notify>();
    authorize_queue = new AsyncQueue<Credentials>();

Scanner::type_signal_update_devices Scanner::signal_update_devices() {
    return m_signal_update_devices;
}

Scanner::type_signal_request_authorisation Scanner::signal_request_authorisation() {
    return m_signal_request_authorisation;
}

Scanner::type_signal_expect_page Scanner::signal_expect_page() {
    return m_signal_expect_page;
}

Scanner::type_signal_got_page_info Scanner::signal_got_page_info() {
    return m_signal_got_page_info;
}

Scanner::type_signal_got_line Scanner::signal_got_line() {
    return m_signal_got_line;
}

Scanner::type_signal_scan_failed Scanner::signal_scan_failed() {
    return m_signal_scan_failed;
}

Scanner::type_signal_page_done Scanner::signal_page_done() {
    return m_signal_page_done;
}

Scanner::type_signal_document_done Scanner::signal_document_done() {
    return m_signal_document_done;
}

Scanner::type_signal_scanning_changed Scanner::signal_scanning_changed() {
    return m_signal_scanning_changed;
}

Scanner Scanner::get_instance(void)
{
    if (scanner_object == NULL)
        Scanner scanner_object;
    return scanner_object;
}

bool Scanner::notify_idle_cb(void)
{
    Notify notification = notify_queue.pop();
    notification.run(this);
    return false;
}

void Scanner::notify_event(Notify notification)
{
    notify_queue.push(notification);
    Idle.add(notify_idle_cb);
}

void Scanner::set_scanning(bool is_scanning)
{
    if ((scanning && !is_scanning) || (!scanning && is_scanning))
    {
        scanning = is_scanning;
        NotifyScanningChanged nsc;
        notify_event(nsc);
    }
}

int Scanner::get_device_weight(std::string device)
{
    /* NOTE: This is using trends in the naming of SANE devices, SANE should be able to provide this information better */

    /* Use webcams as a last resort */
    if (device.has_prefix("vfl:"))
        return 2;

    /* Use locally connected devices first */
    if (device.contains("usb"))
        return 0;

    return 1;
}

int Scanner::compare_devices(ScanDevice device1, ScanDevice device2)
{
    /* TODO: Should do some fuzzy matching on the last selected device and set that to the default */

    int weight1 = get_device_weight(device1.name);
    int weight2 = get_device_weight(device2.name);
    if (weight1 != weight2)
        return weight1 - weight2;

    return strcmp(device1.label, device2.label);
}

void Scanner::do_redetect(void)
{
    SANE_Device ***device_list;
    int status = sane_get_devices(device_list, false);
    spdlog::debug("sane_get_devices () -> {}", sane_str_status(status));
    if (status != SANE_STATUS_GOOD)
    {
        spdlog::warn("Unable to get SANE devices: {}", sane_str_status(status));
        need_redetect = false;
        state = ScanState.IDLE;
        return;
    }

    /* Determine the number of each model to additionally display the name if the model names are the same. */
    std::unordered_map<std::string, int> seen = {
        {str_hash, str_equal}};
    for (int i = 0; device_list[i] != NULL; i++)
    {
        if (seen.contains(device_list[i].model))
        {
            seen.set(device_list[i].model, seen.get(device_list[i].model) + 1);
        }
        else
        {
            seen.set(device_list[i].model, 1);
        }
    }

    std::list<ScanDevice> devices;
    for (int i = 0; device_list[i] != NULL; i++)
    {
        /* Virtual devices tend to not be scanners. Skip them. */
        if (strcmp(device_list[i].type, "virtual device"))
            continue;

        spdlog::debug("Device: name=\"{}\" vendor=\"{}\" model=\"{}\" type=\"{}\"",
                      device_list[i].name, device_list[i].vendor, device_list[i].model, device_list[i].type);

        ScanDevice scan_device = ScanDevice();
        scan_device.name = device_list[i].name;

        /* Abbreviate HP as it is a long string and does not match what is on the physical scanner */
        std::string vendor = device_list[i].vendor;
        if (strcmp(vendor, "Hewlett-Packard"))
            vendor = "HP";

        /* Don't repeat vendor name */
        if (device_list[i].model.down().has_prefix(vendor.down()))
            scan_device.label = device_list[i].model;
        else
            scan_device.label = "%s %s".printf(vendor, device_list[i].model);

        /* Replace underscores in name */
        scan_device.label = scan_device.label.replace("_", " ");

        /* Additionally add the device name to the label if there are several identical models. */
        if (seen.get(device_list[i].model) > 1)
            scan_device.label = "%s on %s".printf(scan_device.label, device_list[i].name);

        devices.append(scan_device);
    }

    /* Sort devices by priority */
    devices.sort(compare_devices);

    need_redetect = false;
    state = ScanState.IDLE;

    if (devices != NULL)
    {
        device = devices.front();
        default_device = device.name;
    }
    else
    {
        default_device = NULL;
    }

    notify_event(NotifyUpdateDevices(devices));
}

double Scanner::scale_fixed(int source_min, int source_max, SANE_Option_Descriptor option, int value)
{
    double v = (double)value;

    return_val_if_fail(option.type == SANE_TYPE_FIXED, value);
    if (option.constraint_type == SANE_CONSTRAINT_RANGE && option.range.max != option.range.min)
    {
        v -= (double)source_min;
        v *= SANE_UNFIX(option.range.max) - SANE_UNFIX(option.range.min);
        v /= (double)(source_max - source_min);
        v += SANE_UNFIX(option.range.min);
        spdlog::debug("scale_fixed: scaling {d} [min: {d}, max: {d}] to {f} [min: {f}, max: {f}]",
                      value, source_min, source_max, v, SANE_UNFIX(option.range.min), SANE_UNFIX(option.range.max));
    }

    return v;
}

int Scanner::scale_int(int source_min, int source_max, SANE_Option_Descriptor option, int value)
{
    int v = value;

    return_val_if_fail(option.type == SANE_TYPE_INT, value);

    if (option.constraint_type == SANE_CONSTRAINT_RANGE && option.range.max != option.range.min)
    {
        v -= source_min;
        v *= (int)(option.range.max - option.range.min);
        v /= (source_max - source_min);
        v += (int)option.range.min;
        spdlog::debug("scale_int: scaling {d} [min: {d}, max: {d}] to {d} [min: {d}, max: {d}]",
                      value, source_min, source_max, v, (int)option.range.min, (int)option.range.max);
    }

    return v;
}

bool Scanner::set_default_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index)
{
    /* Check if supports automatic option */
    if ((option.cap & SANE_CAP_AUTOMATIC) == 0)
        return false;

    int status = sane_control_option(handle, option_index, SANE_ACTION_SET_AUTO, NULL, NULL);
    spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_AUTO, {}=auto) -> {}", (int)option_index, option.name, sane_status_to_string(status));
    if (status != SANE_STATUS_GOOD)
        spdlog::warn("Error setting default option %s: %s", option.name, sane_strstatus(status));

    return status == SANE_STATUS_GOOD;
}

void Scanner::set_bool_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, bool value, bool *result)
{
    return_if_fail(option.type == SANE_TYPE_BOOL);

    SANE_Bool v = (SANE_Bool)value;
    SANE_Int status = sane_control_option(handle, option_index, SANE_ACTION_SET_VALUE, &v, NULL);
    *result = (bool)v;
    spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {s}={s}) -> ({s}, {s})", (int)option_index, option.name, value ? "SANE_TRUE" : "SANE_FALSE", sane_status_to_string(status), result ? "SANE_TRUE" : "SANE_FALSE");
}

void Scanner::set_int_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, int value, int *result)
{
    return_if_fail(option.type == SANE_TYPE_INT);

    SANE_Int v = (SANE_Int)value;
    if (option.constraint_type == SANE_TYPE_RANGE)
    {
        if (option.range.quant != 0)
            v *= option.range.quant;
        if (v < option.range.min)
            v = option.range.min;
        if (v > option.range.max)
            v = option.range.max;
    }
    else if (option.constraint_type == SANE_CONSTRAINT_WORD_LIST)
    {
        int distance = int.MAX, nearest = 0;

        /* Find nearest value to requested */
        for (int i = 0; i < option.word_list[0]; i++)
        {
            int x = (int)option.word_list[i + 1];
            int d = (int)(x - v);
            d = d.abs();
            if (d < distance)
            {
                distance = d;
                nearest = x;
            }
        }
        v = (SANE_Int)nearest;
    }

    SANE_Int status = sane_control_option(handle, option_index, SANE_ACTION_SET_VALUE, &v, NULL);
    spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {s}={d}) -> ({s}, {d})", (int)option_index, option.name, value, sane_status_to_string(status), (int)v);
    result = (int)v;
}

void Scanner::set_fixed_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, double value, double *result)
{
    double v = value;
    SANE_Fixed v_fixed;

    return_if_fail(option.type == SANE_TYPE_FIXED);

    if (option.constraint_type == SANE_TYPE_RANGE)
    {
        double min = SANE_UNFIX(option.range.min);
        double max = SANE_UNFIX(option.range.max);

        if (v < min)
            v = min;
        if (v > max)
            v = max;
    }
    else if (option.constraint_type == SANE_TYPE_WORD_LIST)
    {
        double distance = double.MAX, nearest = 0.0;

        /* Find nearest value to requested */
        for (int i = 0; i < option.word_list[0]; i++)
        {
            double x = SANE_UNFIX(option.word_list[i + 1]);
            if (std::fabs(x - v) < distance)
            {
                distance = std::fabs(x - v);
                nearest = x;
            }
        }
        v = nearest;
    }

    v_fixed = SANE_FIX(v);
    SANE_Int status = sane_control_option(handle, option_index, SANE_ACTION_ET_VALUE, &v_fixed, NULL);
    spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {}={f}) -> ({}, {f})", (int)option_index, option.name, value, sane_status_to_string(status), SANE_UNFIX(v_fixed));

    result = SANE_UNFIX(v_fixed);
}

void Scanner::set_fixed_or_int_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, double value, double *result)
{
    if (option.type == SANE_TYPE_FIXED)
        set_fixed_option(handle, option, option_index, value, out result);
    else if (option.type == SANE_TYPE_INT)
    {
        int r;
        set_int_option(handle, option, option_index, (int)std::round(value), out r);
        result = r;
    }
    else
    {
        result = 0.0;
        spdlog::warn("Unable to set unsupported option type");
    }
}

void Scanner::set_option_to_max(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index)
{
    if (option.constraint_type != SANE_TYPE_RANGE)
        return;

    SANE_Int status = sane_control_option(handle, option_index, SANE_ACTION_SET_VALUE, &option.range.max, NULL);

    if (option.type == SANE_TYPE_FIXED)
        spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {s}=option.range.max={f}) -> ({s})", (int)option_index, option.name, SANE_UNFIX(option.range.max), sane_status_to_string(status));
    else
        spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {s}=option.range.max={d}) -> ({s})", (int)option_index, option.name, (int)option.range.max, sane_status_to_string(status));
}

bool Scanner::set_string_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, std::string value, std::string *result)
{
    result = "";

    return_val_if_fail(option.type == SANE_TYPE_STRING, false);

    char s[option.size];
    int i = 0;
    for (; i < (option.size - 1) && value[i] != '\0'; i++)
        s[i] = value[i];
    s[i] = '\0';
    SANE_Int status = sane_control_option(handle, option_index, SANE_ACTION_SET_VALUE, s, NULL);
    result = (std::string)s;
    spdlog::debug("sane_control_option ({d}, SANE_ACTION_SET_VALUE, {}=\"{}\") -> ({}, \"{}\")", (int)option_index, option.name, value, sane_status_to_string(status), result);

    return status == SANE_STATUS_GOOD;
}

bool Scanner::set_constrained_string_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, std::string values[], std::string *result)
{
    return_val_if_fail(option.type == SANE_TYPE_STRING, false);
    return_val_if_fail(option.constraint_type == SANE_CONSTRAINT_STRING_LIST, false);

    for (int i = 0; values[i] != NULL; i++)
    {
        int j = 0;
        for (; option.string_list[j] != NULL; j++)
        {
            if (values[i] == option.string_list[j])
                break;
        }

        if (option.string_list[j] != NULL)
            return set_string_option(handle, option, option_index, values[i], &result);
    }

    result = "";
    return false;
}

void Scanner::log_option(SANE_Int index, SANE_Option_Descriptor option)
{
    std::string s = "Option %d:".printf((int)index);

    if (!strcmp(option.name, ""))
        s += " name='%s'".printf(option.name);

    if (!strcmp(option.title, ""))
        s += " title='%s'".printf(option.title);

    switch (option.type)
    {
    case SANE_TYPE_BOOL:
        s += " type=bool";
        break;
    case SANE_TYPE_INT:
        s += " type=int";
        break;
    case SANE_TYPE_FIXED:
        s += " type=fixed";
        break;
    case SANE_TYPE_STRING:
        s += " type=string";
        break;
    case SANE_TYPE_BUTTON:
        s += " type=button";
        break;
    case SANE_TYPE_GROUP:
        s += " type=group";
        break;
    default:
        s += " type=%d".printf(option.type);
        break;
    }

    s += " size=%d".printf((int)option.size);

    switch (option.unit)
    {
    case SANE_UNIT_NONE:
        break;
    case SANE_UNIT_PIXEL:
        s += " unit=pixels";
        break;
    case SANE_UNIT_BIT:
        s += " unit=bits";
        break;
    case SANE_UNIT_MM:
        s += " unit=mm";
        break;
    case SANE_UNIT_DPI:
        s += " unit=dpi";
        break;
    case SANE_UNIT_PERCENT:
        s += " unit=percent";
        break;
    case SANE_UNIT_MICROSECOND:
        s += " unit=microseconds";
        break;
    default:
        s += " unit=%d".printf(option.unit);
        break;
    }

    switch (option.constraint_type)
    {
    case SANE_CONSTRAINT_RANGE:
        if (option.range != NULL)
        {
            if (option.type == SANE_TYPE_FIXED)
                s += " min=%f, max=%f, quant=%d".printf(SANE_UNFIX(option.range.min), SANE_UNFIX(option.range.max), (int)option.range.quant);
            else
                s += " min=%d, max=%d, quant=%d".printf((int)option.range.min, (int)option.range.max, (int)option.range.quant);
        }
        break;
    case SANE_CONSTRAINT_WORD_LIST:
        s += " values=[";
        for (int i = 0; i < option.word_list[0]; i++)
        {
            if (i != 0)
                s += ", ";
            if (option.type == SANE_TYPE_INT)
                s += "%d".printf((int)option.word_list[i + 1]);
            else
                s += "%f".printf(SANE_UNFIX(option.word_list[i + 1]));
        }
        s += "]";
        break;
    case SANE_CONSTRAINT_STRING_LIST:
        s += " values=[";
        if (option.string_list != NULL)
        {
            for (int i = 0; option.string_list[i] != NULL; i++)
            {
                if (i != 0)
                    s += ", ";
                s += "\"%s\"".printf(option.string_list[i]);
            }
        }
        s += "]";
        break;
    default:
        break;
    }

    SANE_Int cap = option.cap;
    if (cap != 0)
    {
        std::string caps = "";
        if ((cap & SANE_CAP_SOFT_SELECT) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "soft-select";
            cap &= ~SANE_CAP_SOFT_SELECT;
        }
        if ((cap & SANE_CAP_HARD_SELECT) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "hard-select";
            cap &= ~SANE_CAP_HARD_SELECT;
        }
        if ((cap & SANE_CAP_SOFT_DETECT) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "soft-detect";
            cap &= ~SANE_CAP_SOFT_DETECT;
        }
        if ((cap & SANE_CAP_EMULATED) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "emulated";
            cap &= ~SANE_CAP_EMULATED;
        }
        if ((cap & SANE_CAP_AUTOMATIC) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "automatic";
            cap &= ~SANE_CAP_AUTOMATIC;
        }
        if ((cap & SANE_CAP_INACTIVE) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "inactive";
            cap &= ~SANE_CAP_INACTIVE;
        }
        if ((cap & SANE_CAP_ADVANCED) != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "advanced";
            cap &= ~SANE_CAP_ADVANCED;
        }
        /* Unknown capabilities */
        if (cap != 0)
        {
            if (caps != "")
                caps += ",";
            caps += "%x".printf((int)cap);
        }
        s += " cap=" + caps;
    }

    spdlog::debug("%s", s);

    if (option.desc != NULL)
        spdlog::debug("  Description: %s", option.desc);
}

void Scanner::authorization_cb(std::string resource, char username[], char password[])
{
    scanner_object.notify_event(NotifyRequestAuthorization(resource));

    Credentials credentials = scanner_object.authorize_queue.pop();
    for (int i = 0; credentials.username[i] != '\0' && i < SANE_MAX_USERNAME_LEN; i++)
        username[i] = credentials.username[i];
    for (int i = 0; credentials.password[i] != '\0' && i < SANE_MAX_PASSWORD_LEN; i++)
        password[i] = credentials.password[i];
}

void Scanner::authorize(std::string username, std::string password)
{
    Credentials credentials;
    credentials.username = username;
    credentials.password = password;
    authorize_queue.push(credentials);
}

void Scanner::close_device(void)
{
    if (have_handle)
    {
        sane_cancel(handle);
        spdlog::debug("sane_cancel ()");

        sane_close(handle);
        spdlog::debug("sane_close ()");
        have_handle = false;
        options = NULL;
    }

    buffer = NULL;
    job_queue = NULL;

    set_scanning(false);
}

void Scanner::fail_scan(int error_code, std::string error_string)
{
    close_device();
    state = ScanState.IDLE;
    notify_event(NotifyScanFailed(error_code, error_string));
}

bool Scanner::handle_requests()
{
    /* Redetect when idle */
    if (state == ScanState.IDLE && need_redetect)
        state = ScanState.REDETECT;

    /* Process all requests */
    int request_count = 0;
    while (true)
    {
        Request request;
        if ((state == ScanState.IDLE && request_count == 0) ||
            request_queue.length() > 0)
            request = request_queue.pop();
        else
            return true;

        spdlog::debug("Processing request");
        request_count++;

        // TODO: Fix
        if (request is RequestStartScan)
        {
            RequestStartScan r = (RequestStartScan)request;
            job_queue.append(r.job);
        }
        else if (request is RequestCancel)
        {
            fail_scan(SANE_STATUS_CANCELLED, "Scan cancelled - do not report this error");
        }
        else if (request is RequestQuit)
        {
            close_device();
            return false;
        }
    }
}

void Scanner::do_open()
{
    ScanJob job = (ScanJob)job_queue.data;

    line_count = 0;
    pass_number = 0;
    page_number = 0;
    notified_page = -1;
    option_index = 0;

    if (job.device == NULL && default_device != NULL)
        job.device = default_device;

    if (job.device == NULL)
    {
        spdlog::warn("No scan device available");
        fail_scan(0,
                  /* Error displayed when no scanners to scan with */
                  _("No scanners available.  Please connect a scanner."));
        return;
    }

    /* See if we can use the already open device */
    if (have_handle)
    {
        if (current_device == job.device)
        {
            state = ScanState.GET_OPTION;
            return;
        }

        SANE_close(handle);
        debug("sane_close ()");
        have_handle = false;
    }

    current_device = NULL;

    have_handle = false;
    options = new HashTable<string, int>(str_hash, str_equal);
    var status = SANE_open(job.device, out handle);
    debug("sane_open (\"%s\") -> %s", job.device, SANE_status_to_string(status));

    if (status != SANE_STATUS_GOOD)
    {
        spdlog::warn("Unable to open device: %s", SANE_strstatus(status));
        fail_scan(status,
                  /* Error displayed when cannot connect to scanner */
                  _("Unable to connect to scanner"));
        return;
    }
    have_handle = true;

    current_device = job.device;
    state = ScanState.GET_OPTION;
}

void Scanner::set_adf(ScanJob job, SANE_Option_Descriptor option, SANE_Int index)
{
    char *adf_sources[] =
        {
            "Automatic Document Feeder",
            SANE_I18N("Automatic Document Feeder"),
            "ADF",
            "Automatic Document Feeder(centrally aligned)", /* Seen in the proprietary brother3 driver */
            "Automatic Document Feeder(center aligned)",    /* Seen in Brother's proprietary brscan5 driver */
            "Automatic Document Feeder(left aligned)",      /* Seen in the proprietary brother3 driver */
            "ADF Simplex"                                   /* Samsung unified driver. LP: # 892915 */
        };

    char *adf_front_sources[] =
        {
            "ADF Front",
            SANE_I18N("ADF Front")};

    char *adf_back_sources[] =
        {
            "ADF Back",
            SANE_I18N("ADF Back")};

    char *adf_duplex_sources[] =
        {
            "ADF Duplex",
            "Duplex ADF", /* Brother DS-720, #157 */
            SANE_I18N("ADF Duplex"),
            "ADF Duplex - Long-Edge Binding", /* Samsung unified driver. LP: # 892915 */
            "ADF Duplex - Short-Edge Binding",
            "Duplex",                                              /* HP duplex scan support. LP: #1353599 */
            "Automatic Document Feeder(centrally aligned,Duplex)", /* Brother duplex scan support. LP: #1343773 */
            "Automatic Document Feeder(left aligned,Duplex)"};

    if (job.side == ScanSide.FRONT)
    {
        if (!set_constrained_string_option(handle, option, index, adf_front_sources, NULL))
            if (!set_constrained_string_option(handle, option, index, adf_sources, NULL))
                spdlog::warn("Unable to set front ADF source, please file a bug");
    }
    else if (job.side == ScanSide.BACK)
    {
        if (!set_constrained_string_option(handle, option, index, adf_back_sources, NULL))
            if (!set_constrained_string_option(handle, option, index, adf_sources, NULL))
                spdlog::warn("Unable to set back ADF source, please file a bug");
    }
    else if (job.side == ScanSide.BOTH)
    {
        if (!set_constrained_string_option(handle, option, index, adf_duplex_sources, NULL))
            if (!set_constrained_string_option(handle, option, index, adf_sources, NULL))
                spdlog::warn("Unable to set duplex ADF source, please file a bug");
    }
}

void Scanner::do_get_option(void)
{
    ScanJob job = job_queue.data;

    SANE_Option_Descriptor option = sane_get_option_descriptor(handle, option_index);
    spdlog::debug("sane_get_option_descriptor (%d)", (int)option_index);
    int index = option_index;
    option_index++;

    /* Options complete, apply settings */
    if (option == NULL)
    {
        /* Pick source */
        option = get_option_by_name(handle, SANE_NAME_SCAN_SOURCE, &index);
        if (option == NULL)
        {
            option = get_option_by_name(handle, "doc-source", &index); /* Samsung unified driver. LP: #892915 */
        }
        if (option != NULL)
        {
            std::string flatbed_sources[] =
                {
                    "Auto",
                    SANE_I18N("Auto"),
                    "Flatbed",
                    SANE_I18N("Flatbed"),
                    "FlatBed",
                    "Normal",
                    SANE_I18N("Normal"),
                    "Document Table" /* Epson scanners, eg. ET-3760 */
                };

            switch (job.type)
            {
            case ScanType.SINGLE:
            case ScanType.BATCH:
                if (!set_default_option(handle, option, index))
                    if (!set_constrained_string_option(handle, option, index, flatbed_sources, NULL))
                    {
                        spdlog::warn("Unable to set single page source, trying to set ADF instead");
                        spdlog::warn("If Flatbed is existing and it is not set, please file a bug");
                        set_adf(job, option, index);
                    }
                break;
            case ScanType.ADF:
                set_adf(job, option, index);
                break;
            }
        }

        /* Scan mode (before resolution as it tends to affect that */
        option = get_option_by_name(handle, SANE_NAME_SCAN_MODE, &index);
        if (option != NULL)
        {
            /* The names of scan modes often used in drivers, as taken from the sane-backends source */
            std::string color_scan_modes[] =
                {
                    SANE_VALUE_SCAN_MODE_COLOR,
                    "Color",
                    "24bit Color[Fast]",        /* brother4 driver, Brother DCP-1622WE, #134 */
                    "24bit Color",              /* Seen in the proprietary brother3 driver */
                    "24-bit Color",             /* #161 Lexmark CX310dn */
                    "24 bit Color",             /* brscanads2200ads2700w */
                    "Color - 16 Million Colors" /* Samsung unified driver. LP: 892915 */
                };
            std::string gray_scan_modes[] =
                {
                    SANE_VALUE_SCAN_MODE_GRAY,
                    "Gray",
                    "Grayscale",
                    SANE_I18N("Grayscale"),
                    "8-bit Grayscale",       /* #161 Lexmark CX310dn */
                    "True Gray",             /* Seen in the proprietary brother3 driver */
                    "Grayscale - 256 Levels" /* Samsung unified driver. LP: 892915 */
                };
            std::string lineart_scan_modes[] =
                {
                    SANE_VALUE_SCAN_MODE_LINEART,
                    "Lineart",
                    "LineArt",
                    SANE_I18N("LineArt"),
                    "Black & White",
                    SANE_I18N("Black & White"),
                    "Binary", /* Epson PM-A820 */
                    SANE_I18N("Binary"),
                    "Thresholded",
                    SANE_VALUE_SCAN_MODE_GRAY,
                    "Gray",
                    "Grayscale",
                    SANE_I18N("Grayscale"),
                    "True Gray",                  /* Seen in the proprietary brother3 driver */
                    "1-bit Black & White",        /* #161 Lexmark CX310dn */
                    "Black and White - Line Art", /* Samsung unified driver. LP: 892915 */
                    "Black and White - Halftone",
                    "Monochrome" /* Epson */
                };

            switch (job.scan_mode)
            {
            case ScanMode.COLOR:
                if (!set_constrained_string_option(handle, option, index, color_scan_modes, NULL))
                    spdlog::warn("Unable to set Color mode, please file a bug");
                break;
            case ScanMode.GRAY:
                if (!set_constrained_string_option(handle, option, index, gray_scan_modes, NULL))
                    spdlog::warn("Unable to set Gray mode, please file a bug");
                break;
            case ScanMode.LINEART:
                if (!set_constrained_string_option(handle, option, index, lineart_scan_modes, NULL))
                    spdlog::warn("Unable to set Lineart mode, please file a bug");
                break;
            default:
                break;
            }
        }

        /* Duplex */
        option = get_option_by_name(handle, "duplex", &index);
        if (option == NULL) /* #161 Lexmark CX310dn Duplex */
            option = get_option_by_name(handle, "scan-both-sides", &index);
        if (option != NULL)
        {
            if (option.type == SANE_TYPE_BOOL)
                set_bool_option(handle, option, index, job.side == ScanSide.BOTH, NULL);
        }

        /* Non-standard Epson GT-S50 ADF options */
        option = get_option_by_name(handle, "adf-mode", &index);

        /* Support Canon DR-C240 ADF_BOTH options */
        if (option == NULL)
            option = get_option_by_name(handle, "ScanMode", &index);
        if (option != NULL)
        {
            std::string adf_simplex_modes[] =
                {
                    "Simplex"};
            std::string adf_duplex_modes[] =
                {
                    "Duplex"};
            if (job.side == ScanSide.BOTH)
                set_constrained_string_option(handle, option, index, adf_duplex_modes, NULL);
            else
                set_constrained_string_option(handle, option, index, adf_simplex_modes, NULL);
        }
        option = get_option_by_name(handle, "adf-auto-scan", &index);
        if (option != NULL)
        {
            if (option.type == SANE_TYPE_BOOL)
                set_bool_option(handle, option, index, true, NULL);
        }

        /* Multi-page options */
        option = get_option_by_name(handle, "batch-scan", &index);
        if (option != NULL)
        {
            if (option.type == SANE_TYPE_BOOL)
                set_bool_option(handle, option, index, (job.type != ScanType.SINGLE) && (job.type != ScanType.BATCH), NULL);
        }

        /* Set resolution and bit depth */
        /* Epson may have separate resolution settings for x and y axes, which is preferable options to set */
        option = get_option_by_name(handle, SANE_NAME_SCAN_X_RESOLUTION, &index);
        if (option != NULL && (0 != (option.cap & SANE_CAP_SOFT_SELECT))) // L4160 has non-selectable separate options
        {
            set_fixed_or_int_option(handle, option, index, job.dpi, &job.dpi);
            option = get_option_by_name(handle, SANE_NAME_SCAN_Y_RESOLUTION, &index);
        }
        else
            option = get_option_by_name(handle, SANE_NAME_SCAN_RESOLUTION, &index);
        if (option == NULL) /* #161 Lexmark CX310dn Duplex */
            option = get_option_by_name(handle, "scan-resolution", &index);
        if (option != NULL)
        {
            set_fixed_or_int_option(handle, option, index, job.dpi, &job.dpi);
            option = get_option_by_name(handle, SANE_NAME_BIT_DEPTH, &index);
            if (option != NULL)
            {
                if (job.depth > 0)
                    set_int_option(handle, option, index, job.depth, NULL);
            }
        }

        /* Set scan area */
        option = get_option_by_name(handle, SANE_NAME_SCAN_BR_X, &index);
        if (option != NULL)
        {
            if (job.page_width > 0)
                set_fixed_or_int_option(handle, option, index, convert_page_size(option, job.page_width, job.dpi), NULL);
            else
                set_option_to_max(handle, option, index);
        }
        option = get_option_by_name(handle, SANE_NAME_SCAN_BR_Y, &index);
        if (option != NULL)
        {
            if (job.page_height > 0)
                set_fixed_or_int_option(handle, option, index, convert_page_size(option, job.page_height, job.dpi), NULL);
            else
                set_option_to_max(handle, option, index);
        }
        if (job.page_width == 0)
        {
            /* #90 Fix automatic mode for Epson scanners */
            option = get_option_by_name(handle, "scan-area", &index);
            if (option != NULL)
                set_string_option(handle, option, index, "Maximum", NULL);

            /* #264 Enable automatic document size for Brother scanners */
            option = get_option_by_name(handle, "AutoDocumentSize", &index);
            if (option != NULL)
                set_bool_option(handle, option, index, true, NULL);
        }
        /* Set page size */
        option = get_option_by_name(handle, SANE_NAME_PAGE_WIDTH, &index);
        if (option != NULL && job.page_width > 0.0)
            set_fixed_or_int_option(handle, option, index, convert_page_size(option, job.page_width, job.dpi), NULL);
        option = get_option_by_name(handle, SANE_NAME_PAGE_HEIGHT, out index);
        if (option != NULL && job.page_height > 0.0)
            set_fixed_or_int_option(handle, option, index, convert_page_size(option, job.page_height, job.dpi), NULL);

        option = get_option_by_name(handle, SANE_NAME_BRIGHTNESS, &index);
        if (option != NULL)
        {
            if (option.type == SANE_TYPE_FIXED)
            {
                double brightness = scale_fixed(-100, 100, option, job.brightness);
                set_fixed_option(handle, option, index, brightness, NULL);
            }
            else if (option.type == SANE_TYPE_INT)
            {
                int brightness = scale_int(-100, 100, option, job.brightness);
                set_int_option(handle, option, index, brightness, NULL);
            }
            else
                spdlog::warn("Unable to set brightness, please file a bug");
        }
        option = get_option_by_name(handle, SANE_NAME_CONTRAST, &index);
        if (option != NULL)
        {
            if (option.type == SANE_TYPE_FIXED)
            {
                double contrast = scale_fixed(-100, 100, option, job.contrast);
                set_fixed_option(handle, option, index, contrast, NULL);
            }
            else if (option.type == SANE_TYPE_INT)
            {
                int contrast = scale_int(-100, 100, option, job.contrast);
                set_int_option(handle, option, index, contrast, NULL);
            }
            else
                spdlog::warn("Unable to set contrast, please file a bug");
        }

        /* Test scanner options (hoping will not effect other scanners...) */
        if (current_device == "test")
        {
            option = get_option_by_name(handle, "hand-scanner", &index);
            if (option != NULL)
                set_bool_option(handle, option, index, false, NULL);
            option = get_option_by_name(handle, "three-pass", &index);
            if (option != NULL)
                set_bool_option(handle, option, index, false, NULL);
            option = get_option_by_name(handle, "test-picture", &index);
            if (option != NULL)
                set_string_option(handle, option, index, "Color pattern", NULL);
            option = get_option_by_name(handle, "read-delay", &index);
            if (option != NULL)
                set_bool_option(handle, option, index, true, NULL);
            option = get_option_by_name(handle, "read-delay-duration", &index);
            if (option != NULL)
                set_int_option(handle, option, index, 200000, NULL);
        }

        state = ScanState.START;
        return;
    }

    log_option(index, option);

    /* Ignore groups */
    if (option.type == SANE_TYPE_GROUP)
        return;

    /* Some options are unnamed (e.g. Option 0) */
    if (option.name == NULL)
        return;

    options.insert(option.name, (int)index);
}

double Scanner::convert_page_size(SANE_Option_Descriptor option, double size, double dpi)
{
    if (option.unit == SANE_UNIT_PIXEL)
        return dpi * size / 254.0;
    else if (option.unit == SANE_UNIT_MM)
        return size / 10.0;
    else
    {
        spdlog::warn("Unable to set unsupported unit type");
        return 0.0f;
    }
}

SANE_OptionDescriptor Scanner::get_option_by_name(SANE_Handle handle, std::string name, int *index)
{
    index = options.lookup(name);
    if (index == 0)
        return NULL;

    SANE_Option_Descriptor option_descriptor = sane_get_option_descriptor(handle, index);
    /*
    The SANE_CAP_INACTIVE capability indicates that
    the option is not currently active (e.g., because it's meaningful
    only if another option is set to some other value).
    */
    if ((option_descriptor.cap & SANE_CAP_INACTIVE) != 0)
    {
        spdlog::warn("The option %s (%d) is inactive and can't be set, please file a bug", name, index);
        return NULL;
    }
    return option_descriptor;
}

void Scanner::do_complete_document(void)
{
    sane_cancel(handle);
    spdlog::debug("sane_cancel ()");

    job_queue.remove_link(job_queue);

    state = ScanState.IDLE;

    /* Continue onto the next job */
    if (job_queue != NULL)
    {
        state = ScanState.OPEN;
        return;
    }

    /* Trigger timeout to close */
    // TODO

    NotifyDocumentDone ndd;
    notify_event(ndd);
    set_scanning(false);
}

void Scanner::do_start(void)
{
    SANE_Status status;

    NotifyExpectPage nep;
    notify_event(nep);

    status = sane_start(handle);
    spdlog::debug("sane_start (page=%d, pass=%d) -> %s", page_number, pass_number, SANE_status_to_string(status));
    if (status == SANE_STATUS_GOOD)
        state = ScanState.GET_PARAMETERS;
    else if (status == SANE_STATUS_NO_DOCS)
    {
        do_complete_document();
        if (page_number == 0)
            fail_scan(status,
                      /* Error displayed when no documents at the start of scanning */
                      _("Document feeder empty"));
    }
    else if (status == SANE_STATUS_NO_MEM)
    {
        fail_scan(status,
                  /* Out of memory error message with help instruction.
                     Message written in Pango text markup language,
                     A carriage return makes a line break, <tt> tag makes a monospace font */
                  _("Insufficient memory to perform scan.\n" +
                    "Try to decrease <tt>Resolution</tt> or <tt>Page Size</tt> in <tt>Preferences</tt> menu. " +
                    "For some scanners when scanning in high resolution, the scan size is restricted."));
    }
    else if (status == SANE_STATUS_DEVICE_BUSY)
    {
        /* If device is busy don't interrupt, but keep waiting for scanner */
    }
    else
    {
        spdlog::warn("Unable to start device: %s", sane_strstatus(status));
        fail_scan(status,
                  /* Error display when unable to start scan */
                  _("Unable to start scan"));
    }
}

void Scanner::do_get_parameters(void)
{
    SANE_Parameters status = sane_get_parameters(handle, &parameters);
    spdlog::debug("sane_get_parameters () -> %s", sane_status_to_string(status));
    if (status != SANE_STATUS_GOOD)
    {
        spdlog::warn("Unable to get device parameters: %s", sane_strstatus(status));
        fail_scan(status,
                  /* Error displayed when communication with scanner broken */
                  _("Error communicating with scanner"));
        return;
    }

    ScanJob job = job_queue.data;

    spdlog::debug("Parameters: format=%s last_frame=%s bytes_per_line=%d pixels_per_line=%d lines=%d depth=%d",
          sane_frame_to_string(parameters.format),
          parameters.last_frame ? "SANE_TRUE" : "SANE_FALSE",
          parameters.bytes_per_line,
          parameters.pixels_per_line,
          parameters.lines,
          parameters.depth);

    ScanPageInfo info;
    info.width = parameters.pixels_per_line;
    info.height = parameters.lines;
    info.depth = parameters.depth;
    /* Reduce bit depth if requested lower than received */
    // FIXME: This a hack and only works on 8 bit gray to 2 bit gray
    if (parameters.depth == 8 && parameters.format == SANE_Frame.GRAY && job.depth == 2 && job.scan_mode == ScanMode.GRAY)
        info.depth = job.depth;
    info.n_channels = parameters.format == SANE_Frame.GRAY ? 1 : 3;
    info.dpi = job.dpi; // FIXME: This is the requested DPI, not the actual DPI
    info.device = current_device;

    if (page_number != notified_page)
    {
        NotifyGotPageInfo ngpi(job.id, info)
        notify_event(ngpi);
        notified_page = page_number;
    }

    /* Prepare for read */
    SANE_Int buffer_size = parameters.bytes_per_line + 1; /* Use +1 so buffer is not resized if driver returns one line per read */
    buffer = new uchar[buffer_size];
    n_used = 0;
    line_count = 0;
    pass_number = 0;
    state = ScanState.READ;
}

void Scanner::do_complete_page(void)
{
    ScanJob job = job_queue.data;

    NotifyPageDone npd(job.id);
    notify_event(npd);

    /* If multi-pass then scan another page */
    if (!parameters.last_frame)
    {
        pass_number++;
        state = ScanState.START;
        return;
    }

    /* Go back for another page */
    if (job.type != ScanType.SINGLE)
    {
        if (job.type == ScanType.BATCH)
            Thread.usleep(job.page_delay * 1000);

        page_number++;
        pass_number = 0;
        state = ScanState.START;
        return;
    }

    do_complete_document();
}

void Scanner::do_read(void)
{
    ScanJob job = job_queue.data;

    /* Read as many bytes as we expect */
    int n_to_read = buffer.length - n_used;

    SANE_Int n_read;
    uchar b = (uchar *)buffer;
    SANE_Status status = sane_read(handle, (uint8[])(b + n_used), (SANE_Int)n_to_read, &n_read);
    spdlog::debug("sane_read (%d) -> (%s, %d)", n_to_read, sane_status_to_string(status), (int)n_read);

    /* Completed read */
    if (status == SANE_STATUS_EOF)
    {
        if (parameters.lines > 0 && line_count != parameters.lines)
            spdlog::warn("Scan completed with %d lines, expected %d lines", line_count, parameters.lines);
        if (n_used > 0)
            spdlog::warn("Scan complete with %d bytes of unused data", n_used);
        do_complete_page();
        return;
    }

    /* Some ADF scanners only return NO_DOCS after a read */
    if (status == SANE_STATUS_NO_DOCS)
    {
        do_complete_document();
        if (page_number == 0)
            fail_scan(status,
                      /* Error displayed when no documents at the start of scanning */
                      _("Document feeder empty"));
        return;
    }

    /* Communication error */
    if (status != SANE_STATUS_GOOD)
    {
        spdlog::warn("Unable to read frame from device: %s", sane_strstatus(status));
        fail_scan(status,
                  /* Error displayed when communication with scanner broken */
                  _("Error communicating with scanner"));
        return;
    }

    bool full_read = false;
    if (n_used == 0 && n_read == buffer.length)
        full_read = true;
    n_used += (int)n_read;

    /* Feed out lines */
    if (n_used >= parameters.bytes_per_line)
    {
        var line = new ScanLine();
        switch (parameters.format)
        {
        case SANE_Frame.GRAY:
            line.channel = 0;
            break;
        case SANE_Frame.RGB:
            line.channel = -1;
            break;
        case SANE_Frame.RED:
            line.channel = 0;
            break;
        case SANE_Frame.GREEN:
            line.channel = 1;
            break;
        case SANE_Frame.BLUE:
            line.channel = 2;
            break;
        }
        line.width = parameters.pixels_per_line;
        line.depth = parameters.depth;
        line.data = buffer;
        line.data_length = parameters.bytes_per_line;
        line.number = line_count;
        line.n_lines = n_used / line.data_length;

        line_count += line.n_lines;

        /* Increase buffer size if did full read */
        itn buffer_size = line.data.length;
        if (full_read)
            buffer_size += parameters.bytes_per_line;

        buffer = new uchar[buffer_size];
        int n_remaining = n_used - (line.n_lines * line.data_length);
        n_used = 0;
        for (int i = 0; i < n_remaining; i++)
        {
            buffer[i] = line.data[i + (line.n_lines * line.data_length)];
            n_used++;
        }

        /* Reduce bit depth if requested lower than received */
        // FIXME: This a hack and only works on 8 bit gray to 2 bit gray
        if (parameters.depth == 8 && parameters.format == SANE_Frame.GRAY &&
            job.depth == 2 && job.scan_mode == ScanMode.GRAY)
        {
            uchar block = 0;
            int write_offset = 0;
            int block_shift = 6;
            for (int i = 0; i < line.n_lines; i++)
            {
                int offset = i * line.data_length;
                for (int x = 0; x < line.width; x++)
                {
                    int p = line.data[offset + x];

                    uchar sample;
                    if (p >= 192)
                        sample = 3;
                    else if (p >= 128)
                        sample = 2;
                    else if (p >= 64)
                        sample = 1;
                    else
                        sample = 0;

                    block |= sample << block_shift;
                    if (block_shift == 0)
                    {
                        line.data[write_offset] = block;
                        write_offset++;
                        block = 0;
                        block_shift = 6;
                    }
                    else
                        block_shift -= 2;
                }

                /* Finish each line on a byte boundary */
                if (block_shift != 6)
                {
                    line.data[write_offset] = block;
                    write_offset++;
                    block = 0;
                    block_shift = 6;
                }
            }

            line.data_length = (line.width * 2 + 7) / 8;
        }

        NotifyGotLine ngl(job.id, line);
        notify_event(ngl);
    }
}

void *Scanner::scan_thread(void)
{
    state = ScanState.IDLE;

    SANE_Int version_code;
    SANE_Status status = sane_init(&version_code, authorization_cb);
    spdlog::debug("sane_init () -> %s", sane_status_to_string(status));
    if (status != SANE_STATUS_GOOD)
    {
        spdlog::warn("Unable to initialize SANE backend: %s", sane_strstatus(status));
        return NULL;
    }
    spdlog::debug("SANE version %d.%d.%d",
          SANE_VERSION_MAJOR(version_code),
          SANE_VERSION_MINOR(version_code),
          SANE_VERSION_BUILD(version_code));

    /* Scan for devices on first start */
    redetect();

    while (handle_requests())
    {
        switch (state)
        {
        case ScanState.IDLE:
            if (job_queue != NULL)
            {
                set_scanning(true);
                state = ScanState.OPEN;
            }
            break;
        case ScanState.REDETECT:
            do_redetect();
            break;
        case ScanState.OPEN:
            do_open();
            break;
        case ScanState.GET_OPTION:
            do_get_option();
            break;
        case ScanState.START:
            do_start();
            break;
        case ScanState.GET_PARAMETERS:
            do_get_parameters();
            break;
        case ScanState.READ:
            do_read();
            break;
        }
    }

    return NULL;
}

void Scanner::start(void)
{
    try
    {
        thread = new Thread<void *>.try("scan-thread", scan_thread);
    }
    catch (Error e)
    {
        critical("Unable to create thread: %s", e.message);
    }
}

void Scanner::redetect(void)
{
    if (need_redetect)
        return;
    need_redetect = true;

    spdlog::debug("Requesting redetection of scan devices");

    request_queue.push(new RequestRedetect());
}

bool Scanner::is_scanning(void)
{
    return scanning;
}

std::string Scanner::get_scan_mode_string(ScanMode mode)
{
    switch (mode)
    {
    case ScanMode.DEFAULT:
        return "ScanMode.DEFAULT";
    case ScanMode.COLOR:
        return "ScanMode.COLOR";
    case ScanMode.GRAY:
        return "ScanMode.GRAY";
    case ScanMode.LINEART:
        return "ScanMode.LINEART";
    default:
        return "%d".printf(mode);
    }
}

std::string Scanner::type_to_string(ScanType type)
{
    switch (type)
    {
    case ScanType.SINGLE:
        return "single";
    case ScanType.BATCH:
        return "batch";
    case ScanType.ADF:
        return "adf";
    default:
        return "%d".printf(type);
    }
}

ScanType Scanner::type_from_string(std::string type)
{
    switch (type)
    {
    case "single":
        return ScanType.SINGLE;
    case "batch":
        return ScanType.BATCH;
    case "adf":
        return ScanType.ADF;
    default:
        spdlog::warn("Unknown ScanType: %s. Please report this error.", type);
        return ScanType.SINGLE;
    }
}

std::string Scanner::side_to_string(ScanSide side)
{
    switch (side)
    {
    case ScanSide.FRONT:
        return "front";
    case ScanSide.BACK:
        return "back";
    case ScanSide.BOTH:
        return "both";
    default:
        return "%d".printf(side);
    }
}

ScanSide Scanner::side_from_string(std::string side)
{
    switch (side)
    {
    case "front":
        return ScanSide.FRONT;
    case "back":
        return ScanSide.BACK;
    case "both":
        return ScanSide.BOTH;
    default:
        spdlog::warn("Unknown ScanSide: %s. Please report this error.", side);
        return ScanSide.FRONT;
    }
}

void Scanner::scan(std::string device, ScanOptions options)
{
    spdlog::debug("Scanner.scan (\"%s\", dpi=%d, scan_mode=%s, depth=%d, type=%s, side=%s, paper_width=%d, paper_height=%d, brightness=%d, contrast=%d, delay=%dms)",
          device != NULL ? device : "(NULL)", options.dpi, get_scan_mode_string(options.scan_mode), options.depth,
          type_to_string(options.type), side_to_string(options.side),
          options.paper_width, options.paper_height,
          options.brightness, options.contrast, options.page_delay);
    RequestStartScan request;
    request.job = new ScanJob();
    request.job.id = job_id++;
    request.job.device = device;
    request.job.dpi = options.dpi;
    request.job.scan_mode = options.scan_mode;
    request.job.depth = options.depth;
    request.job.type = options.type;
    request.job.side = options.side;
    request.job.page_width = options.paper_width;
    request.job.page_height = options.paper_height;
    request.job.brightness = options.brightness;
    request.job.contrast = options.contrast;
    request.job.page_delay = options.page_delay;
    request_queue.push(request);
}

void Scanner::cancel(void)
{
    first_job_id = job_id;
    request_queue.push(new RequestCancel());
}

void Scanner::free(void)
{
    spdlog::debug("Stopping scan thread");

    request_queue.push(new RequestQuit());

    if (thread != NULL)
    {
        thread.join();
        thread = NULL;
    }

    sane_exit();
    spdlog::debug("sane_exit ()");
}

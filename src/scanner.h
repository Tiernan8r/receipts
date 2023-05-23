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
#ifndef SCANNER_H
#define SCANNER_H

#include <iostream>
#include <string>
#include <list>
#include <thread>
#include <queue>
#include <map>
#include <vector>
#include <sane/sane.h>
#include <sigc++-3.0/sigc++/sigc++.h>

class ScanDevice
{
public:
    std::string name;
    std::string label;
};

class ScanPageInfo
{
public:
    /* Width, height in pixels */
    int width;
    int height;
    /* Bit depth */
    int depth;
    /* Number of colour channels */
    int n_channels;
    /* Resolution */
    double dpi;
    /* The device this page came from */
    std::string device;
};

class ScanLine
{
public:
    /* Line number */
    int number;
    /* Number of lines in this packet */
    int n_lines;
    /* Width in pixels and format */
    int width;
    int depth;
    /* Channel for this line or -1 for all channels */
    int channel;
    /* Raw line data */
    std::vector<SANE_Byte> data;
    int data_length;
};

enum ScanMode
{
    DEFAULT,
    COLOR,
    GRAY,
    LINEART
};

enum ScanType
{
    SINGLE,
    ADF,
    BATCH
};

enum ScanSide
{
    FRONT,
    BACK,
    BOTH
};

class ScanOptions
{
public:
    int dpi;
    ScanMode scan_mode;
    int depth;
    ScanType type;
    ScanSide side;
    int paper_width;
    int paper_height;
    int brightness;
    int contrast;
    int page_delay;
};

class ScanJob
{
public:
    int id;
    std::string *device;
    double dpi;
    ScanMode scan_mode;
    int depth;
    ScanType type;
    ScanSide side;
    int page_width;
    int page_height;
    int brightness;
    int contrast;
    int page_delay;

    ScanJob(int id, std::string *device, double dpi, ScanMode scan_mode, int depth, ScanType type, ScanSide side, int page_width, int page_height, int brightness, int contrast, int page_delay) : id(id), device(device), dpi(dpi), scan_mode(scan_mode), depth(depth), type(type), side(side), page_width(page_width), page_height(page_height), brightness(brightness), contrast(contrast), page_delay(page_delay){};
};

enum RequestType
{
    BASE,
    REDECT,
    CANCEL,
    START,
    QUIT,
};

class Request
{
public:
    RequestType type = RequestType::BASE;

    Request() : type(RequestType::BASE){};
    Request(RequestType type) : type(type){};
    virtual ScanJob content(void);
};

class RequestRedetect : public Request
{
public:
    RequestRedetect() : Request(RequestType::REDECT){};
};

class RequestCancel : public Request
{
public:
    RequestCancel() : Request(RequestType::CANCEL){};
};

class RequestStartScan : public Request
{
public:
    ScanJob job;

    RequestStartScan(ScanJob job) : Request(RequestType::START), job(job){};
    ScanJob content(void) override;
};

class RequestQuit : public Request
{
public:
    RequestQuit() : Request(RequestType::QUIT){};
};

class Credentials
{
public:
    std::string username;
    std::string password;
};

enum ScanState
{
    IDLE = 0,
    REDETECT,
    OPEN,
    GET_OPTION,
    START,
    GET_PARAMETERS,
    READ
};

class Notify
{
public:
    virtual void run(Scanner *scanner);
};

class NotifyScanningChanged : public Notify
{
public:
    void run(Scanner *scanner) override;
};

class NotifyUpdateDevices : public Notify
{
public:
    NotifyUpdateDevices(std::list<ScanDevice> devices);
    void run(Scanner *scanner) override;

private:
    std::list<ScanDevice> devices;
};

class NotifyRequestAuthorisation : public Notify
{
public:
    NotifyRequestAuthorisation(std::string resource);
    void run(Scanner *scanner) override;

private:
    std::string resource;
};

class NotifyScanFailed : public Notify
{
public:
    NotifyScanFailed(int error_code, std::string error_string);
    void run(Scanner *scanner) override;

private:
    int error_code;
    std::string error_string;
};

class NotifyDocumentDone : public Notify
{
public:
    void run(Scanner *scanner) override;
};

class NotifyExpectPage : public Notify
{
public:
    void run(Scanner *scanner) override;
};

class NotifyGotPageInfo : public Notify
{
public:
    NotifyGotPageInfo(int job_id, ScanPageInfo info);
    void run(Scanner *scanner) override;

private:
    int job_id;
    ScanPageInfo info;
};

class NotifyPageDone : public Notify
{
public:
    NotifyPageDone(int job_id);
    void run(Scanner *scanner) override;

private:
    int job_id;
};

class NotifyGotLine : public Notify
{
public:
    NotifyGotLine(int job_id, ScanLine line);
    void run(Scanner *scanner) override;

private:
    int job_id;
    ScanLine line;
};

class Scanner
{
private:
    /* Singleton object */
    static Scanner *scanner_object_ptr;

    /* Thread communicating with SANE */
    std::thread *thread;

    /* Queue of requests from main thread */
    // AsyncQueue<Request> request_queue;
    std::queue<Request> request_queue;

    /* Queue of events to notify in main queue */
    // AsyncQueue<Notify> notify_queue;
    std::queue<Notify> notify_queue;

    /* Queue of responses to authorisation requests */
    // AsyncQueue<Credentials> authorise_queue;
    std::queue<Credentials> authorise_queue;

    std::string *default_device;

    ScanState state;
    bool need_redetect;

    std::vector<ScanJob> job_queue;

    /* Handle to SANE device */
    SANE_Handle handle;
    bool have_handle;
    std::string *current_device;

    SANE_Parameters parameters;

    /* Last option read */
    SANE_Int option_index;

    /* Table of options */
    std::map<std::string, SANE_Int> options;

    /* Buffer for received line */
    std::vector<SANE_Byte> buffer;
    int n_used;

    //  int bytes_remaining;
    int line_count;
    int pass_number;
    int page_number;
    int notified_page;

    bool scanning;

    // void Scanner::Scanner (void);
    bool notify_idle_cb(void);
    void notify_event(Notify notification);
    void set_scanning(bool is_scanning);
    static int get_device_weight(std::string device);
    static int compare_devices(ScanDevice device1, ScanDevice device2);
    void do_redetect(void);
    double scale_fixed(int source_min, int source_max, SANE_Option_Descriptor option, int value);
    int scale_int(int source_min, int source_max, SANE_Option_Descriptor option, int value);
    bool set_default_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index);
    void set_bool_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, bool value, bool *result);
    void set_int_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, int value, int *result);
    void set_fixed_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, double value, double *result);
    void set_fixed_or_int_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, double value, double *result);
    void set_option_to_max(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index);
    bool set_string_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, std::string value, std::string *result);
    bool set_constrained_string_option(SANE_Handle handle, SANE_Option_Descriptor option, SANE_Int option_index, std::string values[], std::string *result);
    void log_option(SANE_Int index, SANE_Option_Descriptor option);
    static void authorisation_cb(SANE_String_Const resource, SANE_Char username[], SANE_Char password[]);
    void close_device(void);
    void fail_scan(int error_code, std::string error_string);
    bool handle_requests(void);
    void do_open(void);
    void set_adf(ScanJob job, SANE_Option_Descriptor option, SANE_Int index);
    void do_get_option(void);
    double convert_page_size(SANE_Option_Descriptor option, double size, double dpi);
    const SANE_Option_Descriptor *get_option_by_name(SANE_Handle handle, std::string name, int *index);
    void do_complete_document(void);
    void do_start(void);
    void do_get_parameters(void);
    void do_complete_page(void);
    void do_read(void);
    // void *scan_thread (void);
    void scan_thread(void);

public:
    /* ID for the current job */
    int first_job_id;
    int job_id;

    using type_signal_update_devices = sigc::signal<void(std::list<ScanDevice>)>;
    type_signal_update_devices signal_update_devices();
    using type_signal_request_authorisation = sigc::signal<void(std::string)>;
    type_signal_request_authorisation signal_request_authorisation();
    using type_signal_expect_page = sigc::signal<void(void)>;
    type_signal_expect_page signal_expect_page();
    using type_signal_got_page_info = sigc::signal<void(ScanPageInfo)>;
    type_signal_got_page_info signal_got_page_info();
    using type_signal_got_line = sigc::signal<void(ScanLine)>;
    type_signal_got_line signal_got_line();
    using type_signal_scan_failed = sigc::signal<void(int, std::string)>;
    type_signal_scan_failed signal_scan_failed();
    using type_signal_page_done = sigc::signal<void(void)>;
    type_signal_page_done signal_page_done();
    using type_signal_document_done = sigc::signal<void(void)>;
    type_signal_document_done signal_document_done();
    using type_signal_scanning_changed = sigc::signal<void(void)>;
    type_signal_scanning_changed signal_scanning_changed();

    static Scanner *get_instance(void);
    void authorise(std::string username, std::string password);
    void start(void);
    void redetect(void);
    bool is_scanning(void);
    std::string get_scan_mode_string(ScanMode mode);
    static std::string type_to_string(ScanType type);
    static ScanType type_from_string(std::string type);
    static std::string side_to_string(ScanSide side);
    static ScanSide side_from_string(std::string side);
    void scan(std::string *device, ScanOptions options);
    void cancel(void);
    void free(void);

protected:
    type_signal_update_devices m_signal_update_devices;
    type_signal_request_authorisation m_signal_request_authorisation;
    type_signal_expect_page m_signal_expect_page;
    type_signal_got_page_info m_signal_got_page_info;
    type_signal_got_line m_signal_got_line;
    type_signal_scan_failed m_signal_scan_failed;
    type_signal_page_done m_signal_page_done;
    type_signal_document_done m_signal_document_done;
    type_signal_scanning_changed m_signal_scanning_changed;
};

#endif // SCANNER_H
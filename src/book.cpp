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

#include "book.h"
#include "page.h"
#include <string>
#include <queue>

// TODO: figure out delegation...
public
delegate void ProgressionCallback(double fraction);

void Book::page_changed_cb(Page page)
    {
        changed();
    };

    uint Book::n_pages
    {
        get { return pages.length(); }
    }

signal void Book::page_added(Page page);
signal void Book::page_removed(Page page);
signal void Book::reordered();
signal void Book::cleared();
signal void Book::changed();

Book::Book()
    {
        pages = new List<Page>();
    };

Book::~Book(){
        foreach (Page page in pages){
            page.pixels_changed.disconnect(page_changed_cb);
    page.crop_changed.disconnect(page_changed_cb);
}
}


void Book::clear(void)
{
    foreach (Page page in pages)
    {
        page.pixels_changed.disconnect(page_changed_cb);
        page.crop_changed.disconnect(page_changed_cb);
    }
    pages = NULL;
    cleared();
}

void Book::append_page(Page page)
{
    page.pixels_changed.connect(page_changed_cb);
    page.crop_changed.connect(page_changed_cb);

    pages.append(page);
    page_added(page);
    changed();
}

void Book::move_page(Page page, uint location)
{
    pages.remove(page);
    pages.insert(page, (int)location);
    reordered();
    changed();
}

void Book::reverse(void)
{
    std::list<Page> new_pages;
    foreach (Page page in pages)
        new_pages.prepend(page);
    pages = new_pages;

    reordered();
    changed();
}

void Book::combine_sides(void)
{
    int n_front = n_pages - n_pages / 2;
    std::list<Page> new_pages;
    for (int i = 0; i < n_pages; i++)
    {
        if (i % 2 == 0)
            new_pages.append(pages.nth_data(i / 2));
        else
            new_pages.append(pages.nth_data(n_front + (i / 2)));
    }
    pages = new_pages;

    reordered();
    changed();
}

void Book::flip_every_second(FlipEverySecond flip)
{
    std::list<Page> new_pages;
    for (int i = 0; i < n_pages; i++)
    {
        Page page = pages.nth_data(i);
        if (i % 2 == (int)flip)
        {
            page.rotate_left();
            page.rotate_left();
            new_pages.append(page);
        }
        else
        {
            new_pages.append(page);
        }
    }
    pages = new_pages;

    reordered();
    changed();
}

void Book::combine_sides_reverse(void)
{
    std::list<Page> new_pages;
    for (int i = 0; i < n_pages; i++)
    {
        if (i % 2 == 0)
            new_pages.append(pages.nth_data(i / 2));
        else
            new_pages.append(pages.nth_data(n_pages - 1 - (i / 2)));
    }
    pages = new_pages;

    reordered();
    changed();
}

void Book::delete_page(Page page)
{
    page.pixels_changed.disconnect(page_changed_cb);
    page.crop_changed.disconnect(page_changed_cb);
    pages.remove(page);
    page_removed(page);
    changed();
}

Page Book::get_page(int page_number)
{
    if (page_number < 0)
        page_number = (int)pages.length() + page_number;
    return pages.nth_data(page_number);
}

uint Book::get_page_index(Page page)
{
    return pages.index(page);
}

async void Book::save_async(std::string mime_type, int quality, File file,
                      bool postproc_enabled, std::string postproc_script, std::string postproc_arguments, bool postproc_keep_original,
                      ProgressionCallback *progress_cb, Cancellable *cancellable = NULL);
// throws Error
{
    BookSaver book_saver = new BookSaver();
    yield book_saver.save_async(this, mime_type, quality, file,
                                postproc_enabled, postproc_script, postproc_arguments, postproc_keep_original,
                                progress_cb, cancellable);
}

    /* Those methods are run in the encoder threads pool. It process
     * one encode_task issued by save_async and reissue the result with
     * a write_task */

void BookSaver::encode_png(EncodeTask encode_task)
    {
        Page page = encode_task.page;
        var icc_data = page.get_icc_data_encoded();
        WriteTask write_task = new WriteTask();
        var image = page.get_image(true);

        std::string keys[] = {"x-dpi", "y-dpi", "icc-profile", NULL};
        std::string values[] = {"%d".printf(page.dpi), "%d".printf(page.dpi), icc_data, NULL};
        if (icc_data == NULL)
            keys[2] = NULL;

        try
        {
            image.save_to_bufferv(out write_task.data, "png", keys, values);
        }
        catch (Error error)
        {
            write_task.error = error;
        }
        write_task.number = encode_task.number;
        write_queue.push((owned)write_task);

        update_progression();
    };

void BookSaver::encode_jpeg(EncodeTask encode_task)
    {
        Page page = encode_task.page;
        var icc_data = page.get_icc_data_encoded();
        WriteTask write_task = new WriteTask();
        var image = page.get_image(true);

        std::string keys[] = {"x-dpi", "y-dpi", "quality", "icc-profile", NULL};
        std::string values[] = {"%d".printf(page.dpi), "%d".printf(page.dpi), "%d".printf(quality), icc_data, NULL};
        if (icc_data == NULL)
            keys[3] = NULL;

        try
        {
            image.save_to_bufferv(&write_task.data, "jpeg", keys, values);
        }
        catch (Error error)
        {
            write_task.error = error;
        }
        write_task.number = encode_task.number;
        write_queue.push(write_task);

        update_progression();
    };

#if HAVE_WEBP
void BookSaver::encode_webp(owned EncodeTask encode_task)
    {
        Page page = encode_task.page;
        var icc_data = page.get_icc_data_encoded();
        WriteTask write_task = new WriteTask();
        var image = page.get_image(true);
        var webp_data = WebP.encode_rgb(image.get_pixels(),
                                        image.get_width(),
                                        image.get_height(),
                                        image.get_rowstride(),
                                        (float)quality);
#if HAVE_COLORD
        WebP.MuxError mux_error;
        var mux = WebP.Mux.new_mux();
        uint8[] output;

        mux_error = mux.set_image(webp_data, false);
        debug("mux.set_image: %s", mux_error.to_string());

        if (icc_data != NULL)
        {
            mux_error = mux.set_chunk("ICCP", icc_data.data, false);
            debug("mux.set_chunk: %s", mux_error.to_string());
            if (mux_error != WebP.MuxError.OK)
                warning("icc profile data not saved with page %i", encode_task.number);
        }

        mux_error = mux.assemble(out output);
        debug("mux.assemble: %s", mux_error.to_string());
        if (mux_error != WebP.MuxError.OK)
            write_task.error = new FileError.FAILED(_("Unable to encode page %i").printf(encode_task.number));

        write_task.data = (owned)output;
#else

        if (webp_data.length == 0)
            write_task.error = new FileError.FAILED(_("Unable to encode page %i").printf(encode_task.number));

        write_task.data = (owned)webp_data;
#endif
        write_task.number = encode_task.number;
        write_queue.push((owned)write_task);

        update_progression();
    };
#endif

void BookSaver::encode_pdf(EncodeTask encode_task)
    {
        Page page = encode_task.page;
        var image = page.get_image(true);
        int width = image.width;
        int height = image.height;
        uint8 pixels[] = image.get_pixels();
        int depth = 8;
        std::string color_space = "DeviceRGB";
        std::string *filter = NULL;
        uint8 data[];

        if (page.is_color)
        {
            depth = 8;
            color_space = "DeviceRGB";
            int data_length = height * width * 3;
            data = new uint8[data_length];
            for (int row = 0; row < height; row++)
            {
                int in_offset = row * image.rowstride;
                int out_offset = row * width * 3;
                for (int x = 0; x < width; x++)
                {
                    int in_o = in_offset + x * 3;
                    int out_o = out_offset + x * 3;

                    data[out_o] = pixels[in_o];
                    data[out_o + 1] = pixels[in_o + 1];
                    data[out_o + 2] = pixels[in_o + 2];
                }
            }
        }
        else if (page.depth == 2)
        {
            int shift_count = 6;
            depth = 2;
            color_space = "DeviceGray";
            int data_length = height * ((width * 2 + 7) / 8);
            data = new uint8[data_length];
            int offset = 0;
            for (int row = 0; row < height; row++)
            {
                /* Pad to the next line */
                if (shift_count != 6)
                {
                    offset++;
                    shift_count = 6;
                }

                int in_offset = row * image.rowstride;
                for (int x = 0; x < width; x++)
                {
                    /* Clear byte */
                    if (shift_count == 6)
                        data[offset] = 0;

                    /* Set bits */
                    int p = pixels[in_offset + x * 3];
                    if (p >= 192)
                        data[offset] |= 3 << shift_count;
                    else if (p >= 128)
                        data[offset] |= 2 << shift_count;
                    else if (p >= 64)
                        data[offset] |= 1 << shift_count;

                    /* Move to the next position */
                    if (shift_count == 0)
                    {
                        offset++;
                        shift_count = 6;
                    }
                    else
                        shift_count -= 2;
                }
            }
        }
        else if (page.depth == 1)
        {
            int mask = 0x80;

            depth = 1;
            color_space = "DeviceGray";
            int data_length = height * ((width + 7) / 8);
            data = new uint8[data_length];
            int offset = 0;
            for (int row = 0; row < height; row++)
            {
                /* Pad to the next line */
                if (mask != 0x80)
                {
                    offset++;
                    mask = 0x80;
                }

                int in_offset = row * image.rowstride;
                for (int x = 0; x < width; x++)
                {
                    /* Clear byte */
                    if (mask == 0x80)
                        data[offset] = 0;

                    /* Set bit */
                    if (pixels[in_offset + x * 3] != 0)
                        data[offset] |= (uint8)mask;

                    /* Move to the next bit */
                    mask >>= 1;
                    if (mask == 0)
                    {
                        offset++;
                        mask = 0x80;
                    }
                }
            }
        }
        else
        {
            depth = 8;
            color_space = "DeviceGray";
            int data_length = height * width;
            data = new uint8[data_length];
            for (int row = 0; row < height; row++)
            {
                int in_offset = row * image.rowstride;
                int out_offset = row * width;
                for (int x = 0; x < width; x++)
                    data[out_offset + x] = pixels[in_offset + x * 3];
            }
        }

        /* Compress data and use zlib compression if it is smaller than JPEG.
         * zlib compression is slower in the worst case, so do JPEG first
         * and stop zlib if it exceeds the JPEG size */
        WriteTaskPDF write_task = new WriteTaskPDF();
        uint8 *jpeg_data[] = NULL;
        try
        {
            jpeg_data = compress_jpeg(image, quality, page.dpi);
        }
        catch (Error error)
        {
            write_task.error = error;
        }
        var zlib_data = compress_zlib(data, jpeg_data.length);
        if (zlib_data != NULL)
        {
            filter = "FlateDecode";
            data = zlib_data;
        }
        else
        {
            filter = "DCTDecode";
            data = jpeg_data;
        }

        write_task.number = encode_task.number;
        write_task.data = data;
        write_task.width = width;
        write_task.height = height;
        write_task.color_space = color_space;
        write_task.depth = depth;
        write_task.filter = filter;
        write_task.dpi = page.dpi;
        write_queue.push(write_task);

        update_progression();
    };

Error* BookSaver::write_multifile(void)
    {
        for (int i = 0; i < n_pages; i++)
        {
            if (cancellable.is_cancelled())
            {
                finished_saving();
                return NULL;
            }

            WriteTask write_task = write_queue.pop();
            if (write_task.error != NULL)
            {
                finished_saving();
                return write_task.error;
            }

            var indexed_file = make_indexed_file(file.get_uri(), write_task.number, n_pages);
            try
            {
                var stream = indexed_file.replace(NULL, false, FileCreateFlags.NONE);
                stream.write_all(write_task.data, NULL);
            }
            catch (Error error)
            {
                finished_saving();
                return error;
            }
        }

        update_progression();
        finished_saving();
        return NULL;
    };

    /* Those methods are run in the writer thread. It receive all
     * write_tasks sent to it by the encoder threads and write those to
     * disk. */

Error* BookSaver::write_pdf(void)
    {
        /* Generate a random ID for this file */
        std::string id = "";
        for (int i = 0; i < 4; i++)
            id += "%08x".printf(Random.next_int());

        FileOutputStream *stream = NULL;
        try
        {
            stream = file.replace(NULL, false, FileCreateFlags.NONE, NULL);
        }
        catch (Error error)
        {
            finished_saving();
            return error;
        }
        PDFWriter writer = new PDFWriter(stream);

        /* Choose object numbers */
        uint catalog_number = writer.add_object();
        uint metadata_number = writer.add_object();
        uint pages_number = writer.add_object();
        uint info_number = writer.add_object();
        uint page_numbers[] = new uint[n_pages];
        uint page_image_numbers[] = new uint[n_pages];
        uint page_content_numbers[] = new uint[n_pages];
        for (int i = 0; i < n_pages; i++)
        {
            page_numbers[i] = writer.add_object();
            page_image_numbers[i] = writer.add_object();
            page_content_numbers[i] = writer.add_object();
        }
        uint struct_tree_root_number = writer.add_object();

        /* Header */
        writer.write_string("%PDF-1.3\n");

        /* Comment with binary as recommended so file is treated as binary */
        writer.write_string("%\xe2\xe3\xcf\xd3\n");

        /* Catalog */
        writer.start_object(catalog_number);
        writer.write_string("%u 0 obj\n".printf(catalog_number));
        writer.write_string("<<\n");
        writer.write_string("/Type /Catalog\n");
        writer.write_string("/Metadata %u 0 R\n".printf(metadata_number));
        writer.write_string("/MarkInfo << /Marked true >>\n");
        writer.write_string("/StructTreeRoot %u 0 R\n".printf(struct_tree_root_number));
        writer.write_string("/Pages %u 0 R\n".printf(pages_number));
        writer.write_string(">>\n");
        writer.write_string("endobj\n");

        /* Metadata */
        var now = new DateTime.now_local();
        std::string date_string = now.format("%FT%H:%M:%S%:z");
        /* NOTE: The id has to be hardcoded to this value according to the spec... */
        std::string metadata = '<?xpacket begin="%s" id="W5M0MpCehiHzreSzNTczkc9d"?> < rdf : RDF xmlns : rdf = "http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns : xmp = "http://ns.adobe.com/xap/1.0/" >
                                                                                                                                                                           < rdf : Description rdf : about = "" xmlns : pdfaid = "http://www.aiim.org/pdfa/ns/id/" xmlns : xmp = "http://ns.adobe.com/xap/1.0/" >
                                                                                                                                                                                                                                                                                 < pdfaid : part > 1 < / pdfaid : part >
                                                                                                                                                                                                                                                                                            < pdfaid : conformance > A < / pdfaid : conformance >
                                                                                                                                                                                                                                                                                                       < xmp : CreatorTool > Simple Scan % s < / xmp : CreatorTool >
                                                                                                                                                                                                                                                                                                               < xmp : CreateDate > % s < / xmp : CreateDate >
                                                                                                                                                                                                                                                                                                                       < xmp : ModifyDate > % s < / xmp : ModifyDate >
                                                                                                                                                                                                                                                                                                                               < xmp : MetadataDate > % s < / xmp : MetadataDate >
                                                                                                                                                                                                                                                                                                                                       < / rdf : Description >
                                                                                                                                                                                                                                                                                                                                       < / rdf : RDF >
                                                                                                                                                                                                                                                                                                                                       <
                                                                                                                                                                                                                                                                           ? xpacket end = "w" ? >'.printf (((unichar) 0xFEFF).to_string (), VERSION, date_string, date_string, date_string); writer.write_string("\n");
        writer.start_object(metadata_number);
        writer.write_string("%u 0 obj\n".printf(metadata_number));
        writer.write_string("<<\n");
        writer.write_string("/Type /Metadata\n");
        writer.write_string("/Subtype /XML\n");
        writer.write_string("/Length %u\n".printf(metadata.length));
        writer.write_string(">>\n");
        writer.write_string("stream\n");
        writer.write_string(metadata);
        writer.write_string("\n");
        writer.write_string("endstream\n");
        writer.write_string("endobj\n");

        /* Pages */
        writer.write_string("\n");
        writer.start_object(pages_number);
        writer.write_string("%u 0 obj\n".printf(pages_number));
        writer.write_string("<<\n");
        writer.write_string("/Type /Pages\n");
        writer.write_string("/Kids [");
        for (int i = 0; i < n_pages; i++)
            writer.write_string(" %u 0 R".printf(page_numbers[i]));
        writer.write_string(" ]\n");
        writer.write_string("/Count %u\n".printf(n_pages));
        writer.write_string(">>\n");
        writer.write_string("endobj\n");

        /* Process each page in order */
        std::queue<WriteTaskPDF> tasks_in_standby;
        for (int i = 0; i < n_pages; i++)
        {
            if (cancellable.is_cancelled())
            {
                finished_saving();
                return NULL;
            }

            WriteTask write_task = tasks_in_standby.peek_head();
            if (write_task != NULL && write_task.number == i)
                tasks_in_standby.pop_head();
            else
            {
                while (true)
                {
                    write_task = (WriteTaskPDF)write_queue.pop();
                    if (write_task.error != NULL)
                    {
                        finished_saving();
                        return write_task.error;
                    }
                    if (write_task.number == i)
                        break;

                    tasks_in_standby.insert_sorted(
                        write_task, (a, b) = > { return a.number - b.number; });
                }
            }

            double page_width = write_task.width * 72.0 / write_task.dpi;
            double page_height = write_task.height * 72.0 / write_task.dpi;
            char width_buffer[] = new char[double.DTOSTR_BUF_SIZE];
            char height_buffer[] = new char[double.DTOSTR_BUF_SIZE];

            /* Page */
            writer.write_string("\n");
            writer.start_object(page_numbers[i]);
            writer.write_string("%u 0 obj\n".printf(page_numbers[i]));
            writer.write_string("<<\n");
            writer.write_string("/Type /Page\n");
            writer.write_string("/Parent %u 0 R\n".printf(pages_number));
            writer.write_string("/Resources << /XObject << /Im%d %u 0 R >> >>\n".printf(i, page_image_numbers[i]));
            writer.write_string("/MediaBox [ 0 0 %s %s ]\n".printf(page_width.format(width_buffer, "%.2f"), page_height.format(height_buffer, "%.2f")));
            writer.write_string("/Contents %u 0 R\n".printf(page_content_numbers[i]));
            writer.write_string(">>\n");
            writer.write_string("endobj\n");

            /* Page image */
            writer.write_string("\n");
            writer.start_object(page_image_numbers[i]);
            writer.write_string("%u 0 obj\n".printf(page_image_numbers[i]));
            writer.write_string("<<\n");
            writer.write_string("/Type /XObject\n");
            writer.write_string("/Subtype /Image\n");
            writer.write_string("/Width %d\n".printf(write_task.width));
            writer.write_string("/Height %d\n".printf(write_task.height));
            writer.write_string("/ColorSpace /%s\n".printf(write_task.color_space));
            writer.write_string("/BitsPerComponent %d\n".printf(write_task.depth));
            writer.write_string("/Length %d\n".printf(write_task.data.length));
            if (write_task.filter != NULL)
                writer.write_string("/Filter /%s\n".printf(write_task.filter));
            writer.write_string(">>\n");
            writer.write_string("stream\n");
            writer.write(write_task.data);
            writer.write_string("\n");
            writer.write_string("endstream\n");
            writer.write_string("endobj\n");

            /* Structure tree */
            writer.write_string("\n");
            writer.start_object(struct_tree_root_number);
            writer.write_string("%u 0 obj\n".printf(struct_tree_root_number));
            writer.write_string("<<\n");
            writer.write_string("/Type /StructTreeRoot\n");
            writer.write_string(">>\n");
            writer.write_string("endobj\n");

            /* Page contents */
            std::string command = "q\n%s 0 0 %s 0 0 cm\n/Im%d Do\nQ".printf(page_width.format(width_buffer, "%f"), page_height.format(height_buffer, "%f"), i);
            writer.write_string("\n");
            writer.start_object(page_content_numbers[i]);
            writer.write_string("%u 0 obj\n".printf(page_content_numbers[i]));
            writer.write_string("<<\n");
            writer.write_string("/Length %d\n".printf(command.length));
            writer.write_string(">>\n");
            writer.write_string("stream\n");
            writer.write_string(command);
            writer.write_string("\n");
            writer.write_string("endstream\n");
            writer.write_string("endobj\n");
        }

        /* Info */
        writer.write_string("\n");
        writer.start_object(info_number);
        writer.write_string("%u 0 obj\n".printf(info_number));
        writer.write_string("<<\n");
        writer.write_string("/Creator (Simple Scan %s)\n".printf(VERSION));
        writer.write_string(">>\n");
        writer.write_string("endobj\n");

        /* Cross-reference table */
        writer.write_string("\n");
        int xref_offset = writer.offset;
        writer.write_string("xref\n");
        writer.write_string("0 %zu\n".printf(writer.object_offsets.length + 1));
        writer.write_string("%010zu 65535 f \n".printf(writer.next_empty_object(0)));
        for (int i = 0; i < writer.object_offsets.length; i++)
            if (writer.object_offsets[i] == 0)
                writer.write_string("%010zu 65535 f \n".printf(writer.next_empty_object(i + 1)));
            else
                writer.write_string("%010zu 00000 n \n".printf(writer.object_offsets[i]));

        /* Trailer */
        writer.write_string("\n");
        writer.write_string("trailer\n");
        writer.write_string("<<\n");
        writer.write_string("/Size %zu\n".printf(writer.object_offsets.length + 1));
        writer.write_string("/Info %u 0 R\n".printf(info_number));
        writer.write_string("/Root %u 0 R\n".printf(catalog_number));
        writer.write_string("/ID [<%s> <%s>]\n".printf(id, id));
        writer.write_string(">>\n");
        writer.write_string("startxref\n");
        writer.write_string("%zu\n".printf(xref_offset));
        writer.write_string("%%EOF\n");

        update_progression();
        finished_saving();
        return NULL;
    };

    /* update_progression is called once by page by encoder threads and
     * once at the end by writer thread. */
void BookSaver::update_progression(void)
    {
        double step = 1.0 / (double)(n_pages + 1);
        progression_mutex.lock();
        progression += step;
        progression_mutex.unlock();
        Idle.add(() = >
                      {
                          progression_callback(progression);
                          return false;
                      });
    }

    /* finished_saving is called by the writer thread when it's done,
     * meaning there is nothing left to do or saving has been
     * cancelled */
void BookSaver::finished_saving(void)
    {
        /* At this point, any remaining encode_task ought to remain unprocessed */
        ThreadPool.free(encoder, true, true);

        /* Wake-up save_async method in main thread */
        Idle.add(save_async_callback);
    }

    /* Utility methods */

*uint8[] BookSaver::compress_zlib(uint8[] data, uint max_size)
    {
        var stream = ZLib::DeflateStream(ZLib::Level.BEST_COMPRESSION);
        uint8 out_data[] = new uint8[max_size];

        stream.next_in = data;
        stream.next_out = out_data;
        while (true)
        {
            /* Compression complete */
            if (stream.avail_in == 0)
                break;

            /* Out of space */
            if (stream.avail_out == 0)
                return NULL;

            if (stream.deflate(ZLib.Flush.FINISH) == ZLib.Status.STREAM_ERROR)
                return NULL;
        }

        int n_written = out_data.length - stream.avail_out;
        out_data.resize((int)n_written);

        return out_data;
    };

uint8[] BookSaver::compress_jpeg(Gdk::Pixbuf image, int quality, int dpi); // throws Error
    {
        uint8 jpeg_data[];
        std::string keys[] = {"quality", "x-dpi", "y-dpi", NULL};
        std::string values[] = {"%d".printf(quality), "%d".printf(dpi), "%d".printf(dpi), NULL};

        image.save_to_bufferv(&jpeg_data, "jpeg", keys, values);
        return jpeg_data;
    };

    /* save_async get called in the main thread to start saving. It
     * distributes all encode tasks to other threads then yield so
     * the ui can continue operating. The method then return once saving
     * is completed, cancelled, or failed */
async void BookSaver::save_async(Book book, std::string mime_type, int quality, File file,
                          bool postproc_enabled, std::string postproc_script, std::string postproc_arguments, bool postproc_keep_original,
                          ProgressionCallback *progression_callback, Cancellable *cancellable);
    // throws Error
    {
        Timer timer = new Timer();

        this.n_pages = book.n_pages;
        this.quality = quality;
        this.file = file;
        this.cancellable = cancellable;
        this.save_async_callback = save_async.callback;
        this.write_queue = new AsyncQueue<WriteTask>();
        this.progression = 0;
        this.progression_mutex = Mutex();

        /* Configure a callback that monitor saving progression */
        if (progression_callback == NULL)
            this.progression_callback = (fraction) = >
            {
                debug("Save progression: %f%%", fraction * 100.0);
            };
        else
            this.progression_callback = progression_callback;

        /* Configure an encoder */
        ThreadPoolFunc<EncodeTask> ? encode_delegate = NULL;
        switch (mime_type)
        {
        case "image/jpeg":
            encode_delegate = encode_jpeg;
            break;
        case "image/png":
            encode_delegate = encode_png;
            break;
#if HAVE_WEBP
        case "image/webp":
            encode_delegate = encode_webp;
            break;
#endif
        case "application/pdf":
            encode_delegate = encode_pdf;
            break;
        }
        encoder = new ThreadPool<EncodeTask>.with_owned_data(encode_delegate, (int)get_num_processors(), false);

        /* Configure a writer */
        Thread < Error ? > writer;

        switch (mime_type)
        {
        case "image/jpeg":
        case "image/png":
#if HAVE_WEBP
        case "image/webp":
#endif
            writer = new Thread < Error ? > (NULL, write_multifile);
            break;
        case "application/pdf":
            writer = new Thread < Error ? > (NULL, write_pdf);
            break;
        default:
            writer = new Thread < Error ? > (NULL, () = > NULL);
            break;
        }

        /* Issue encode tasks */
        for (var i = 0; i < n_pages; i++)
        {
            var encode_task = new EncodeTask();
            encode_task.number = i;
            encode_task.page = book.get_page(i);
            encoder.add((owned)encode_task);
        }

        /* Waiting for saving to finish */
        yield;

        /* Any error from any thread ends up here */
        var error = writer.join();
        if (error != NULL)
            throw error;

        timer.stop();
        debug("Save time: %f seconds", timer.elapsed(NULL));

        if (postproc_enabled)
        {
            /* Perform post-processing */
            timer = new Timer();
            var return_code = postprocessor.process(postproc_script,
                                                    mime_type,              // MIME Type
                                                    postproc_keep_original, // Keep Original
                                                    file.get_path(),        // Filename
                                                    postproc_arguments      // Arguments
            );
            if (return_code != 0)
            {
                warning("Postprocessing script execution failed. ");
            }
            timer.stop();
            debug("Postprocessing time: %f seconds", timer.elapsed(NULL));
        }
    };

PDFWriter::PDFWriter(FileOutputStream stream)
    {
        this.stream = stream;
        object_offsets = new uint[0];
    };

void PDFWriter::write(uint8 data[])
    {
        try
        {
            stream.write_all(data, NULL, NULL);
        }
        catch (Error e)
        {
            warning("Error writing PDF: %s", e.message);
        }
        offset += data.length;
    };

void PDFWriter::write_string(std::string text)
    {
        write((uint8[])text.to_utf8());
    };

uint PDFWriter::add_object(void)
    {
        object_offsets.resize(object_offsets.length + 1);
        int index = object_offsets.length - 1;
        object_offsets[index] = 0;
        return index + 1;
    };

void PDFWriter::start_object(uint index)
    {
        object_offsets[index - 1] = (uint)offset;
    };

int PDFWriter::next_empty_object(int start)
    {
        for (int i = start; i < object_offsets.length; i++)
            if (object_offsets[i] == 0)
                return i + 1;
        return 0;
    };

File make_indexed_file(std::string uri, uint i, uint n_pages)
{
    if (n_pages == 1)
        return File.new_for_uri(uri);

    /* Insert index before extension */
    std::string basename = Path.get_basename(uri);
    std::string prefix = uri, suffix = "";
    int extension_index = basename.last_index_of_char('.');
    if (extension_index >= 0)
    {
        suffix = basename.slice(extension_index, basename.length);
        prefix = uri.slice(0, uri.length - suffix.length);
    }
    int width = n_pages.to_string().length;
    std::string number_format = "%%0%dd".printf(width);
    std::string filename = prefix + "-" + number_format.printf(i + 1) + suffix;
    return File.new_for_uri(filename);
};

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
#ifndef BOOK_H
#define BOOK_H

#include <list>
#include <sigc++/sigc++.h>
#include "page.h"

public delegate void ProgressionCallback (double fraction);

// Assumes first page has index 0
enum FlipEverySecond {
    Even = 1,
    Odd = 0,
};

class Book
{
    private:
        std::list<Page> pages;
    
        void page_changed_cb (Page page);

    public:
        uint n_pages { get { return pages.length (); } }

        using type_signal_page_added = sigc::signal<void(Page)>;
        type_signal_page_added signal_page_added();
        using type_signal_page_removed = sigc::signal<void(Page)>;
        type_signal_page_removed signal_page_removed();
        using type_signal_reordered = sigc::signal<void(void)>;
        type_signal_reordered signal_reordered();
        using type_signal_cleared = sigc::signal<void(void)>;
        type_signal_cleared signal_cleared();
        using type_signal_changed = sigc::signal<void(void)>;
        type_signal_changed signal_changed();

        Book (void);
        Book (void) override;
        void clear (void);
        void append_page (Page page);
        void move_page (Page page, uint location);
        void reverse (void);
        void combine_sides (void);
        void flip_every_second (FlipEverySecond flip);
        void combine_sides_reverse (void);
        void delete_page (Page page);
        Page get_page (int page_number);
        uint get_page_index (Page page);
        std::async void save_async (std::string mime_type, int quality, File file,
        bool postproc_enabled, std::string postproc_script, std::string postproc_arguments, bool postproc_keep_original,
        ProgressionCallback? progress_cb, Cancellable? cancellable = null); throws Error

    protected:
        type_signal_page_added m_signal_page_added;
        type_signal_page_removed m_signal_page_removed;
        type_signal_reordered m_signal_reordered;
        type_signal_cleared m_signal_cleared;
        type_signal_changed m_signal_changed;
};

class BookSaver
{
    private:
        uint n_pages;
        int quality;
        File file;
        unowned ProgressionCallback progression_callback;
        double progression;
        Mutex progression_mutex;
        Cancellable? cancellable;
        AsyncQueue<WriteTask> write_queue;
        ThreadPool<EncodeTask> encoder;
        SourceFunc save_async_callback;
        Postprocessor postprocessor;    
    /* Those methods are run in the encoder threads pool. It process
     * one encode_task issued by save_async and reissue the result with
     * a write_task */

        void encode_png (owned EncodeTask encode_task);
        void encode_jpeg (owned EncodeTask encode_task);
#if HAVE_WEBP
        void encode_webp (owned EncodeTask encode_task);
#endif

        void encode_pdf (owned EncodeTask encode_task);
        Error? write_multifile (void);
    /* Those methods are run in the writer thread. It receive all
     * write_tasks sent to it by the encoder threads and write those to
     * disk. */

        Error? write_pdf (void);
    /* update_progression is called once by page by encoder threads and
     * once at the end by writer thread. */
        void update_progression (void);
    /* finished_saving is called by the writer thread when it's done,
     * meaning there is nothing left to do or saving has been
     * cancelled */
        void finished_saving (void);
    /* Utility methods */
        static uint8[]? compress_zlib (uint8[] data, uint max_size);
        static uint8[] compress_jpeg (Gdk.Pixbuf image, int quality, int dpi); throws Error

    public:
    /* save_async get called in the main thread to start saving. It
     * distributes all encode tasks to other threads then yield so
     * the ui can continue operating. The method then return once saving
     * is completed, cancelled, or failed */
        std::async void save_async (Book book, std::string mime_type, int quality, File file,
        bool postproc_enabled, std::string postproc_script, std::string postproc_arguments, bool postproc_keep_original,
        ProgressionCallback? progression_callback, Cancellable? cancellable); throws Error

};

class EncodeTask
{
    public:
        int number;
        Page page;
};

class WriteTask
{
    public:
        int number;
        uint8 data[];
        Error error;
};

class WriteTaskPDF : WriteTask
{
    public:
        int width;
        int height;
        std::string color_space;
        int depth;
        std::string? filter;
        int dpi;
};

class PDFWriter
{
    public:
        size_t offset = 0;
        uint object_offsets[];

        PDFWriter (FileOutputStream stream);
        void write (uint8 data[]);
        void write_std::string (std::string text);
        uint add_object (void);
        void start_object (uint index);
        int next_empty_object (int start);
    private:
        FileOutputStream stream;
};

File make_indexed_file (std::string uri, uint i, uint n_pages);

#endif //BOOK_H
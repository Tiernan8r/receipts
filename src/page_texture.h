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
#ifndef PAGE_TEXTURE_H
#define PAGE_TEXTURE_H

#include <gtkmm.h>

class PageToPixbuf
{
    private:
        Gdk::Pixbuf *pixbuf_ = NULL;

    /* Direction of currently scanned image */
        ScanDirection scan_direction;

    /* Next scan line to render */
        int scan_line;

        static uchar get_sample (uchar[] pixels, int offset, int x, int depth, int sample);
        static void get_pixel (Page page, int x, int y, uchar pixel[]);
        static void set_pixel (Page page, double l, double r, double t, double b, uchar[] output, int offset);

    /* Image to render at current resolution */
    public:
        Gdk::Pixbuf *pixbuf { get { return pixbuf_; } };
    /* Dimensions of image to generate */
        int width;
        int height;
        static void update_preview (Page page, ref Gdk::Pixbuf? output_image, int output_width, int output_height,
                                 ScanDirection scan_direction, int old_scan_line, int scan_line);
        void update (Page page);
};

/**
 * Just update texture contents
 */
class TextureUpdateTask
{
    public:
        Page page;
};

/**
 * Resize the texture
 */
class TextureResizeTask: private TextureUpdateTask
{
    public:
        int width { get; private set; };
        int height { get; private set; };
    
	    TextureResizeTask (int width, int height);
};

class PageViewTexture
{

    private:
        int requested_width;
        int requested_height;
        TextureUpdateTask queued = null;

        bool in_progress;

        ThreadPool<TextureUpdateTask> resize_pool;
    
        Page page;
        PageToPixbuf page_view;
        void thread_func(owned TextureUpdateTask task);
        void new_pixbuf_cb (Gdk::Pixbuf? pixbuf);
    
	public:
        Gdk::Pixbuf *pixbuf { get; private set; }
        signal void new_buffer (void);
        PageViewTexture (Page page);
    /**
     * Notify that data needs updating (eg. pixels changed during scanning process)
     */
        void request_update (void);
    /**
     * Set size of the page, ignored if size did not change.
     */
        void request_resize (int width, int height);
        void queue_update (void); throws ThreadError
};

class PagePaintable : public Gdk::Paintable
{
    private:
        Page page;
        PageViewTexture page_texture;
        Gdk::Texture *texture;

        void pixels_changed (void);
        void texture_updated (void);
    
    public:
        void PagePaintable (Page page);
        void PagePaintable (void) override;
	    double get_intrinsic_aspect_ratio (void) override;
        void snapshot (Gdk::Snapshot gdk_snapshot, double width, double height);

};

#endif //PAGE_TEXTURE_H
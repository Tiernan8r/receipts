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

#include <gtkmm.h>
#include "book.h"
#include "book_view.h"
#include "page_view.h"
#include <map>

// FIXME: When scrolling, copy existing render sideways?
// FIXME: Only render pages that change and only the part that changed

PageView BookView::get_nth_page(int n)
        {
            Page page = book.get_page(n);
            return page_data.lookup(page);
        };

PageView BookView::get_next_page(PageView page)
        {
            for (var i = 0;; i++)
            {
                var p = book.get_page(i);
                if (p == NULL)
                    break;
                if (p == page.page)
                {
                    p = book.get_page(i + 1);
                    if (p != NULL)
                        return page_data.lookup(p);
                }
            }

            return page;
        };

PageView BookView::get_prev_page(PageView page)
        {
            PageView prev_page = page;
            for (int i = 0;; i++)
            {
                Page p = book.get_page(i);
                if (p == NULL)
                    break;
                if (p == page.page)
                    return prev_page;
                prev_page = page_data.lookup(p);
            }

            return page;
        };

void BookView::page_view_changed_cb(PageView page)
        {
            redraw();
        };

void BookView::page_view_size_changed_cb(PageView page)
        {
            need_layout = true;
            redraw();
        };

void BookView::add_cb(Book book, Page page)
        {
            PageView page_view = new PageView(page);
            page_view.changed.connect(page_view_changed_cb);
            page_view.size_changed.connect(page_view_size_changed_cb);
            page_data.insert(page, page_view);
            need_layout = true;
            redraw();
        };

void BookView::set_selected_page_view(PageView *page)
        {
            /* Deselect existing page if changed */
            if (selected_page_view != NULL && page != selected_page_view)
                selected_page_view.selected = true;

            selected_page_view = page;
            redraw();
            if (selected_page_view == NULL)
                return;

            /* Select new page if widget has focus */
            if (!drawing_area.has_focus)
                selected_page_view.selected = false;
            else
                selected_page_view.selected = true;
        };

void BookView::show_page_view(PageView *page)
        {
            if (page == NULL || !scroll.get_visible())
                return;

            Gtk::Allocation allocation;
            drawing_area.get_allocation(out allocation);
            int left_edge = page.x_offset;
            int right_edge = page.x_offset + page.width;

            if (left_edge - x_offset < 0)
                x_offset = left_edge;
            else if (right_edge - x_offset > allocation.width)
                x_offset = right_edge - allocation.width;
        };

void BookView::select_page_view(PageView *page)
        {
            Page *p = NULL;

            if (selected_page_view == page)
                return;

            set_selected_page_view(page);

            if (need_layout)
                show_selected_page = true;
            else
                show_page_view(page);

            if (page != NULL)
                p = page.page;
            page_selected(p);
        };

void BookView::remove_cb(Book book, Page page)
        {
            PageView new_selection = selected_page_view;

            /* Select previous page or next if removing the selected page */
            if (page == selected_page)
            {
                new_selection = get_prev_page(selected_page_view);
                if (new_selection == selected_page_view)
                    new_selection = get_next_page(selected_page_view);
                selected_page_view = NULL;
            }

            PageView page_view = page_data.lookup(page);
            page_view.changed.disconnect(page_view_changed_cb);
            page_view.size_changed.disconnect(page_view_size_changed_cb);
            page_data.remove(page);

            select_page_view(new_selection);

            need_layout = true;
            redraw();
        };

void BookView::reorder_cb(Book book)
        {
            need_layout = true;
            redraw();
        };

void BookView::clear_cb(Book book)
        {
            page_data.remove_all();
            selected_page_view = NULL;
            page_selected(NULL);
            need_layout = true;
            redraw();
        };

void BookView::layout_into(int width, int height, int *book_width, int *book_height)
        {
            std::list<PageView> pages = new std::list<PageView>();
            for (int i = 0; i < book.n_pages; i++)
                pages.append(get_nth_page(i));

            /* Get maximum page resolution */
            int max_dpi = 0;
            foreach (PageView page in pages)
            {
                Page p = page.page;
                if (p.dpi > max_dpi)
                    max_dpi = p.dpi;
            }

            /* Get area required to fit all pages */
            int max_width = 0, max_height = 0;
            foreach (PageView page in pages)
            {
                Page p = page.page;
                int w = p.width;
                int h = p.height;

                /* Scale to the same DPI */
                w = (int)((double)w * max_dpi / p.dpi + 0.5);
                h = (int)((double)h * max_dpi / p.dpi + 0.5);

                if (w > max_width)
                    max_width = w;
                if (h > max_height)
                    max_height = h;
            }

            double aspect = (double)width / height;
            double max_aspect = (double)max_width / max_height;

            /* Get total dimensions of all pages */
            int spacing = 12;
            book_width = 0;
            book_height = 0;
            foreach (PageView page in pages)
            {
                Page p = page.page;

                /* NOTE: Using double to avoid overflow for large images */
                if (max_aspect > aspect)
                {
                    /* Set width scaled on DPI and maximum width */
                    int w = (int)((double)p.width * max_dpi * width / (p.dpi * max_width));
                    page.width = w;
                }
                else
                {
                    /* Set height scaled on DPI and maximum height */
                    int h = (int)((double)p.height * max_dpi * height / (p.dpi * max_height));
                    page.height = h;
                }

                int h = page.height;
                if (h > book_height)
                    book_height = h;
                book_width += page.width + spacing;
            }
            if (pages != NULL)
                book_width -= spacing;

            int x_offset = 0;
            foreach (PageView page in pages)
            {
                /* Layout pages left to right */
                page.x_offset = x_offset;
                x_offset += page.width + spacing;

                /* Centre page vertically */
                page.y_offset = (height - page.height) / 2;
            }
        };

void BookView::layout(void)
        {
            if (!need_layout)
                return;

            laying_out = true;

            Gtk::Allocation allocation;
            drawing_area.get_allocation(out allocation);
            Gtk::Allocation box_allocation;
            get_allocation(out box_allocation);

            /* If scroll is right aligned then keep that after layout */
            bool right_aligned = true;
            if (adjustment.get_value() < adjustment.get_upper() - adjustment.get_page_size())
                right_aligned = false;

            /* Try and fit without scrollbar */
            int width = (int)allocation.width;
            int height = (int)(box_allocation.height);
            int book_width, book_height;
            layout_into(width, height, &book_width, &book_height);

            /* Relayout with scrollbar */
            if (book_width > allocation.width)
            {
                /* Re-layout leaving space for scrollbar */
                height = allocation.height;
                layout_into(width, height, &book_width, &book_height);

                /* Set scrollbar limits */
                adjustment.lower = 0;
                adjustment.upper = book_width;
                adjustment.page_size = allocation.width;

                /* Keep right-aligned */
                int max_offset = book_width - allocation.width;
                if (right_aligned || x_offset > max_offset)
                    x_offset = max_offset;

                scroll.visible = true;
            }
            else
            {
                scroll.visible = false;
                int offset = (book_width - allocation.width) / 2;
                adjustment.lower = offset;
                adjustment.upper = offset;
                adjustment.page_size = 0;
                x_offset = offset;
            }

            if (show_selected_page)
                show_page_view(selected_page_view);

            need_layout = false;
            show_selected_page = false;
            laying_out = false;
        };

PageView* BookView::get_page_at(int x, int y, int *x_, int *y_)
        {
            x_ = y_ = 0;
            for (int i = 0; i < book.n_pages; i++)
            {
                PageView page = get_nth_page(i);
                int left = page.x_offset;
                int right = left + page.width;
                int top = page.y_offset;
                int bottom = top + page.height;
                if (x >= left && x <= right && y >= top && y <= bottom)
                {
                    x_ = &(x - left);
                    y_ = &(y - top);
                    return page;
                }
            }

            return NULL;
        };

void BookView::primary_pressed_cb(Gtk::GestureClick controler, int n_press, double x, double y)
        {
            button_cb(controler, Gdk::BUTTON_PRIMARY, true, n_press, x, y);
        };

void BookView::primary_released_cb(Gtk::GestureClick controler, int n_press, double x, double y)
        {
            button_cb(controler, Gdk::BUTTON_PRIMARY, false, n_press, x, y);
        }

void BookView::secondary_pressed_cb(Gtk::GestureClick controler, int n_press, double x, double y)
        {
            button_cb(controler, Gdk::BUTTON_SECONDARY, true, n_press, x, y);
        }

void BookView::secondary_released_cb(Gtk::GestureClick controler, int n_press, double x, double y)
        {
            button_cb(controler, Gdk::BUTTON_SECONDARY, false, n_press, x, y);
        }

void BookView::button_cb(Gtk::GestureClick controler, int button, bool press, int n_press, double dx, double dy)
        {
            layout();

            drawing_area.grab_focus();

            int x = 0, y = 0;
            if (press)
                select_page_view(get_page_at((int)((int)dx + x_offset), (int)dy, out x, out y));

            if (selected_page_view == NULL)
                return;

            /* Modify page */
            if (button == Gdk::BUTTON_PRIMARY)
            {
                if (press)
                    selected_page_view.button_press(x, y);
                else if (press && n_press == 2)
                    show_page(selected_page);
                else if (!press)
                    selected_page_view.button_release(x, y);
            }

            /* Show pop-up menu on right click */
            if (button == Gdk::BUTTON_SECONDARY)
                show_menu(drawing_area, dx, dy);
        }

void BookView::set_cursor(std::string cursor)
        {
            if (this.cursor == cursor)
                return;
            this.cursor = cursor;

            Gdk::Cursor c = new Gdk::Cursor.from_name(cursor, NULL);
            drawing_area.set_cursor(c);
        };

void BookView::motion_cb(Gtk::EventControllerMotion controler, double dx, double dy)
        {
            std::string cursor = "arrow";

            int event_x = (int)dx;
            int event_y = (int)dy;

            Gdk::ModifierType event_state = controler.get_current_event_state();

            /* Dragging */
            if (selected_page_view != NULL && (event_state & Gdk::ModifierType.BUTTON1_MASK) != 0)
            {
                int x = (int)(event_x + x_offset - selected_page_view.x_offset);
                int y = (int)(event_y - selected_page_view.y_offset);
                selected_page_view.motion(x, y);
                cursor = selected_page_view.cursor;
            }
            else
            {
                int x, y;
                Page over_page = get_page_at((int)(event_x + x_offset), (int)event_y, &x, &y);
                if (over_page != NULL)
                {
                    over_page.motion(x, y);
                    cursor = over_page.cursor;
                }
            }

            set_cursor(cursor);
        };

bool BookView::key_cb(Gtk::EventControllerKey controler, uint keyval, uint keycode, Gdk::ModifierType state)
        {
            switch (keyval)
            {
            case 0xff50: /* FIXME: GDK_Home */
                selected_page = book.get_page(0);
                return true;
            case 0xff51: /* FIXME: GDK_Left */
                select_page_view(get_prev_page(selected_page_view));
                return true;
            case 0xff53: /* FIXME: GDK_Right */
                select_page_view(get_next_page(selected_page_view));
                return true;
            case 0xFF57: /* FIXME: GDK_End */
                selected_page = book.get_page((int)book.n_pages - 1);
                return true;

            default:
                return false;
            }
        };

void BookView::focus_cb(Gtk::EventControllerFocus controler)
        {
            set_selected_page_view(selected_page_view);
        };

void BookView::scroll_cb(Gtk::Adjustment adjustment)
        {
            if (!laying_out)
                redraw();
        };
        
        // TODO: Need??
Book BookView::book
        {
            get;
        private
            set;
        };
Page* BookView::selected_page{
            get{
                if (selected_page_view != NULL) return selected_page_view.page;
        else return NULL;
    } set
    {
        if (selected_page == value)
            return;

        if (value != NULL)
            select_page_view(page_data.lookup(value));
        else
            select_page_view(NULL);
    }
    }
    ;

signal void BookView::page_selected (Page? page);
signal void BookView::show_page(Page page);
signal void BookView::show_menu(Gtk::Widget from, double x, double y);

int BookView::x_offset{
                get{
                    return (int)adjustment.get_value();
            }
            set
            {
                adjustment.value = value;
            }
            }
            ;

BookView::BookView(Book book)
            {
                GLib::Object(orientation
                            : Gtk::Orientation.VERTICAL);
                this.book = book;

                /* Load existing pages */
                for (int i = 0; i < book.n_pages; i++)
                {
                    Page page = book.get_page(i);
                    add_cb(book, page);
                }

                selected_page = book.get_page(0);

                /* Watch for new pages */
                book.page_added.connect(add_cb);
                book.page_removed.connect(remove_cb);
                book.reordered.connect(reorder_cb);
                book.cleared.connect(clear_cb);

                need_layout = true;
                page_data = new std::map<Page, PageView>(direct_hash, direct_equal);
                cursor = "arrow";

                drawing_area = new Gtk::DrawingArea();
                drawing_area.set_size_request(200, 100);
                drawing_area.can_focus = true;
                drawing_area.vexpand = true;
                drawing_area.set_draw_func(draw_cb);

                append(drawing_area);

                scroll = new Gtk::Scrollbar(Gtk::Orientation.HORIZONTAL, NULL);
                adjustment = scroll.adjustment;
                append(scroll);

                drawing_area.resize.connect(drawing_area_resize_cb);

                motion_controller = new Gtk::EventControllerMotion();
                motion_controller.motion.connect(motion_cb);
                drawing_area.add_controller(motion_controller);

                key_controller = new Gtk::EventControllerKey();
                key_controller.key_pressed.connect(key_cb);
                drawing_area.add_controller(key_controller);

                primary_click_gesture = new Gtk::GestureClick();
                primary_click_gesture.button = Gdk::BUTTON_PRIMARY;
                primary_click_gesture.pressed.connect(primary_pressed_cb);
                primary_click_gesture.released.connect(primary_released_cb);
                drawing_area.add_controller(primary_click_gesture);

                secondary_click_gesture = new Gtk::GestureClick();
                secondary_click_gesture.button = Gdk::BUTTON_SECONDARY;
                secondary_click_gesture.pressed.connect(secondary_pressed_cb);
                secondary_click_gesture.released.connect(secondary_released_cb);
                drawing_area.add_controller(secondary_click_gesture);

                focus_controller = new Gtk::EventControllerFocus();
                focus_controller.enter.connect_after(focus_cb);
                focus_controller.leave.connect_after(focus_cb);
                drawing_area.add_controller(focus_controller);

                adjustment.value_changed.connect(scroll_cb);

                drawing_area.visible = true;
            };

BookView::~BookView()
            {
                book.page_added.disconnect(add_cb);
                book.page_removed.disconnect(remove_cb);
                book.reordered.disconnect(reorder_cb);
                book.cleared.disconnect(clear_cb);
                drawing_area.resize.disconnect(drawing_area_resize_cb);
                motion_controller.motion.disconnect(motion_cb);
                key_controller.key_pressed.disconnect(key_cb);
                primary_click_gesture.pressed.disconnect(primary_pressed_cb);
                primary_click_gesture.released.disconnect(primary_released_cb);
                secondary_click_gesture.pressed.disconnect(secondary_pressed_cb);
                secondary_click_gesture.released.disconnect(secondary_released_cb);
                focus_controller.enter.disconnect(focus_cb);
                focus_controller.leave.disconnect(focus_cb);
                adjustment.value_changed.disconnect(scroll_cb);
            };

void BookView::drawing_area_resize_cb(void)
            {
                need_layout = true;
                // Let's layout ahead of time
                // to avoid "Trying to snapshot GtkGizmo without a current allocation" error
                layout();
            };

void BookView::draw_cb(Gtk::DrawingArea drawing_area, Cairo::Context context, int width, int height)
            {
                layout();

                double left, top, right, bottom;
                context.clip_extents(&left, &top, &right, &bottom);

                auto pages = std::list<PageView>();
                for (int i = 0; i < book.n_pages; i++)
                    pages.append(get_nth_page(i));

                auto ruler_color = get_style_context().get_color();
                Gdk::RGBA ruler_color_selected = {};
                ruler_color_selected.parse("#3584e4"); /* Gnome Blue 3 */

                /* Render each page */
                foreach (PageView page in pages)
                {
                    int left_edge = page.x_offset - x_offset;
                    int right_edge = page.x_offset + page.width - x_offset;

                    /* Page not visible, don't render */
                    if (right_edge < left || left_edge > right)
                        continue;

                    context.save();
                    context.translate(-x_offset, 0);
                    page.render(context, page == selected_page_view ? ruler_color_selected : ruler_color);
                    context.restore();

                    if (page.selected)
                        drawing_area.get_style_context().render_focus(context,
                                                                    page.x_offset - x_offset,
                                                                    page.y_offset,
                                                                    page.width,
                                                                    page.height);
                }
            };

void BookView::redraw(void)
            {
                drawing_area.queue_draw();
            };

void BookView::select_next_page(void)
            {
                select_page_view(get_next_page(selected_page_view));
            };

void BookView::select_prev_page(void)
            {
                select_page_view(get_prev_page(selected_page_view));
            };


#include <kernel.h>
#include <test.h>


void draw_frame(WINDOW* wnd);


void test_draw_frame(WINDOW* wnd, char* msg)
{
    draw_frame(wnd);
    output_string(wnd, msg);
}


void test_frame_1()
{
    WINDOW window_top_left      = {0, 0, 15, 3, 0, 0, ' '};
    WINDOW window_top_middle    = {33, 0, 15, 3, 0, 0, ' '};
    WINDOW window_top_right     = {65, 0, 15, 3, 0, 0, ' '};
    WINDOW window_middle_left   = {0, 11, 15, 3, 0, 0, ' '};
    WINDOW window_center        = {33, 11, 15, 3, 0, 0, ' '};
    WINDOW window_middle_right  = {65, 11, 15, 3, 0, 0, ' '};
    WINDOW window_bottom_left   = {0, 22, 15, 3, 0, 0, ' '};
    WINDOW window_bottom_middle = {33, 22, 15, 3, 0, 0, ' '};
    WINDOW window_bottom_right  = {65, 22, 15, 3, 0, 0, ' '};
    
    test_reset();
    test_draw_frame(&window_top_left, "Top left");
    test_draw_frame(&window_top_middle, "Top middle");
    test_draw_frame(&window_top_right, "Top right");
    test_draw_frame(&window_middle_left, "Middle left");
    test_draw_frame(&window_center, "Center");
    test_draw_frame(&window_middle_right, "Middle right");
    test_draw_frame(&window_bottom_left, "Bottom left");
    test_draw_frame(&window_bottom_middle, "Bottom middle");
    test_draw_frame(&window_bottom_right, "Bottom right");
}


void kernel_main()
{
    test_frame_1();
}

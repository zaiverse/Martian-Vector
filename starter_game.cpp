// file: triangle_xlib.cpp
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <cstdio>
#include <unistd.h>

int main() {
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) { std::fprintf(stderr, "Failed to open display\n"); return 1; }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    // Create window
    unsigned int w = 640, h = 480;
    Window win = XCreateSimpleWindow(
        dpy, root,
        100, 100, w, h,                 // x, y, width, height
        0,                              // border width
        BlackPixel(dpy, screen),        // border color
        WhitePixel(dpy, screen)         // background color
    );

    // Select events we care about
    XSelectInput(dpy, win, ExposureMask | KeyPressMask | StructureNotifyMask);

    // Title + close button handling
    XStoreName(dpy, win, "Triangle (Xlib) - Use arrow keys to move");
    Atom wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wmDelete, 1);

    // Show the window
    XMapWindow(dpy, win);

    // Create a GC (graphics context)
    GC gc = XCreateGC(dpy, win, 0, nullptr);
    // Foreground color (black lines)
    XSetForeground(dpy, gc, BlackPixel(dpy, screen));

    // Head of character position state
    int circleX = w/2;
    int circleY = h/2;
    const int circleRadius = 50;
    const int circleDiameter = 2 * circleRadius;

    // Triangle position state (body)
    int triangleX = w/2;
    int triangleY = h/2;
    const int triangleSize = 50;
    int moveSpeed = 10;

    bool running = true;
    bool needsRedraw = true;

    while (running) {
        if (needsRedraw) {
            // Clear the window
            XClearWindow(dpy, win);
            
            // Update triangle points based on current position
            XPoint tri[3];
            tri[0].x = triangleX;          tri[0].y = triangleY - triangleSize; // top
            tri[1].x = triangleX - triangleSize; tri[1].y = triangleY + triangleSize; // bottom-left
            tri[2].x = triangleX + triangleSize; tri[2].y = triangleY + triangleSize; // bottom-right
            
            // Draw the triangle
            XFillPolygon(dpy, win, gc, tri, 3, Convex, CoordModeOrigin);

            // Update circle points based on current position
            XFillArc(dpy, win, gc, 
                    circleX - circleRadius,  // x position (top-left)
                    circleY - circleRadius,  // y position (top-left)  
                    circleDiameter, circleDiameter, 0, 360 * 64);
            
            needsRedraw = false;
        }

        // Check for events without blocking
        if (XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);

            switch (ev.type) {
                case Expose:
                    needsRedraw = true;
                    break;

                case ConfigureNotify:
                    // Window resized
                    w = ev.xconfigure.width;
                    h = ev.xconfigure.height;
                    // Keep triangle and circle within bounds
                    if (triangleX > w) triangleX = w - triangleSize;
                    if (triangleY > h) triangleY = h - triangleSize;
                    if (circleX > w) circleX = w - circleRadius;
                    if (circleY > h) circleY = h - circleRadius;
                    needsRedraw = true;
                    break;

                case ClientMessage:
                    if ((Atom)ev.xclient.data.l[0] == wmDelete) {
                        running = false;
                    }
                    break;

                case KeyPress:
                    {
                        KeySym key = XLookupKeysym(&ev.xkey, 0);
                        bool moved = false;
                        
                        switch (key) {
                            case XK_Left:
                            case XK_a:
                                triangleX -= moveSpeed;
                                circleX -= moveSpeed;
                                moved = true;
                                break;
                            case XK_Right:
                            case XK_d:
                                triangleX += moveSpeed;
                                circleX += moveSpeed;
                                moved = true;
                                break;
                            case XK_Up:
                            case XK_w:
                                triangleY -= moveSpeed;
                                circleY -= moveSpeed;
                                moved = true;
                                break;
                            case XK_Down:
                            case XK_s:
                                triangleY += moveSpeed;
                                circleY += moveSpeed;
                                moved = true;
                                break;
                            case XK_Escape:
                                running = false;
                                break;
                            case XK_plus:
                            case XK_equal:
                                moveSpeed += 5;
                                break;
                            case XK_minus:
                                moveSpeed = (moveSpeed > 5) ? moveSpeed - 5 : 1;
                                break;
                        }
                        
                        // Keep triangle within window bounds
                        if (triangleX < triangleSize) triangleX = triangleSize;
                        if (triangleX > w - triangleSize) triangleX = w - triangleSize;
                        if (triangleY < triangleSize) triangleY = triangleSize;
                        if (triangleY > h - triangleSize) triangleY = h - triangleSize;

                        // Keep circle within window bounds
                        if (circleX < circleRadius) circleX = circleRadius;
                        if (circleX > w - circleRadius) circleX = w - circleRadius;
                        if (circleY < circleRadius) circleY = circleRadius;
                        if (circleY > h - circleRadius) circleY = h - circleRadius;
                        
                        if (moved) {
                            needsRedraw = true;
                        }
                    }
                    break;
            }
        } else {
            // No events pending, small delay to prevent CPU spinning
            usleep(10000); // 10ms
        }
    }

    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return 0;
}
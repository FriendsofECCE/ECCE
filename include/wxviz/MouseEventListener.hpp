/**
 * @file 
 *
 *
 */
#ifndef MOUSEEVENTLISTENER_HH
#define MOUSEEVENTLISTENER_HH


class wxMouseEvent;

/**
 * Abstract class that defines interface for objects that wich to
 * receive motion events from the 3D viewer.
 */
class MouseEventListener
{
    public:

        /**
         * Process the mouse event.
         * TODO make return value indicate handled or not
         */
        virtual bool mouseEvent(wxMouseEvent *event) = 0;
};

#endif

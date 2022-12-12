# Chapter 7 - Using Qt in Mu

You can browse the Qt and other Mu modules with the documentation browser. RV wraps the Qt API. Using Qt in Mu is similar to using it in C++. Each Qt class is presented as a Mu class which you can either use directly or inherit from if need be. However, there are some major differences that need to be observed:

> **Note:** Python code can assume that default parameters values will be supplied if not specified.

*   Not all Qt classes are wrapped in Mu. It's a good idea to look in the documentation browser to see if a class is available yet.
*   Property names in C++ do not always match those in Mu. Mu collects Qt properties at runtime in order to provide limited supported for unknown classes. So the set and get functions for the properties are generated at that time. Usually these names match the C++ names, but sometimes there are differences. In general, the Mu function to get a property called **foo** will be called foo(). The Mu function to set the foo property will be called setFoo(). (A good example of this is the QWidget property **visible** . In C++ the get function is isVisible() whereas the Mu function is called visible().) 
*   Templated classes in Qt are not available in Mu. Usually these are handled by dynamic array types or something analogous to the Qt class. In the case of template member functions (like QWidget::findChild<>) there may be an equivalent Mu version that operates slightly differently (like the Mu version QWidget.findChild). 
*   The QString class is not wrapped (yet). Instead, the native Mu string can be used anywhere a function takes a QString.
*   You cannot control widget destruction. If you loose a reference to a QObject it will eventually be finalized (destroyed), but at an unknown time.
*   Some classes cannot be inherited from. You can inherit from any QObject, QPainter, or QLayoutItem derived class except QWebFrame and QNetworkReply.
*   The signal slot mechanism is slightly different in Mu than C++. It is currently not possible to make a new Qt signal, and slots do not need to be declared in a special way (but they do need to have the correct signatures to be connected). In addition, you are not required to create a QObject class to receive a signal in Mu. You can also connect a signal directly to a regular function if desired (as opposed to class member functions in C++).
*   Threading is not yet available. The QThread class cannot be used in Mu yet.
*   Abstract Qt classes can be instantiated. However, you can't really do anything with them.
*   Protected member functions are public.

### 7.1 Signals and Slots


Possibly the biggest difference between the Mu and C++ Qt API is how signals and slots are handled. This discussion will assume knowledge of the C++ mechanism. See the Qt documentation if you don't know what signals and slots are.Jumping right in, here is an example hello world MuQt program. This can be run from the mu-interp binary:

```
 use qt;

\: clicked (void; bool checked)
{
    print("OK BYE\n");
    QCoreApplication.exit(0);
}

\: main ()
{
    let app    = QApplication(string[] {"hello.mu"}),
        window = QWidget(nil, Qt.Window),
        button = QPushButton("MuQt: HELLO WORLD!", window);

    connect(button, QPushButton.clicked, clicked);

    window.setSize(QSize(200, 50));
    window.show();
    window.raise();
    QApplication.exec();
}

main();
 
```
The main thing to notice in this example is the connect() function. A similar C++ version of this would look like this:

```
 connect(button, SIGNAL(clicked(bool)), SLOT(myclickslot(bool))); 
```
where myclickslot would be a slot function declared in a class. In Mu it's not necessary to create a class to receive a signal. In addition the SIGNAL and SLOT syntax is also unnecessary. However, it is necessary to exactly specify which signal is being referred to by passing its Mu function object directly. In this case QPushButton.clicked. The signal must be a function on the class of the first argument of connect().In Mu, any function which matches the signal's signature can be used to receive the signal. The downside of this is that some functions like sender() are not available in Mu. However this is easily overcome with partial application. In the above case, if we need to know who sent the signal in our clicked function, we can change its signature to accept the sender and partially apply it in the connect call like so:

```
\: clicked (void; bool checked, QPushButton sender)
{
    // do something with sender
}

\: main ()
{
    ...

    connect(button, QPushButton.clicked, clicked(,button));
}
 
```
And of course additional information can be passed into the clicked function by applying more arguments.It's also possible to connect a signal to a class method in Mu if the method signature matches. Partial application can be used in that case as well. This is frequently the case when writing a mode which uses Qt interface.

### 7.2 Inheriting from Qt Classes


It's possible to inherit directly from the Qt classes in Mu and override methods. Virtual functions in the C++ version of Qt are translated as class methods in Mu. Non-virtual functions are regular functions in the scope of the class. In practice this means that the Mu Qt class usage is very similar to the C++ usage.The following example shows how to create a new widget type that implements a drop target. Drag and drop is one aspect of Qt that requires inheritance (in C++ and Mu):

```
use qt;

class: MyWidget : QWidget
{
    method: MyWidget (MyWidget; QObject parent, int windowFlags)
    {
        // REQUIRED: call base constructor to build Qt native object
        QWidget.QWidget(this, parent, windowFlags);
        setAcceptDrops(true);
    }

    method: dragEnterEvent (void; QDragEnterEvent event)
    {
        print("drop enter\n");
        event.acceptProposedAction();
    }

    method: dropEvent (void; QDropEvent event)
    {
        print("drop\n");
        let mimeData = event.mimeData(),
            formats = mimeData.formats();

        print("--formats--\n");
        for_each (f; formats) print("%s\n" % f);

        if (mimeData.hasUrls())
        {
            print("--urls--\n");
            for_each (u; event.mimeData().urls())
                print("%s\n" % u.toString(QUrl.None));
        }

        if (mimeData.hasText())
        {
            print("--text--\n");
            print("%s\n" % mimeData.text());
        }

        event.acceptProposedAction();
    }
} 
```
Things to note in this example: the names of the drag and drop methods matter. These are same names as used in C++. If you browser the documentation of a Qt class in Mu these will be the class methods. Only class methods can be overridden.
#ifndef REARK_APPLICATION_CONTROLLER_H
#define REARK_APPLICATION_CONTROLLER_H

#include <QObject>

class ApplicationController : public QObject {
    Q_OBJECT

public:
    explicit ApplicationController(QObject* parent = nullptr);

    Q_INVOKABLE bool openNewWindow();
};

#endif // REARK_APPLICATION_CONTROLLER_H

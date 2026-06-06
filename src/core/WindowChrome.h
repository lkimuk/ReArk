#ifndef REARK_WINDOW_CHROME_H
#define REARK_WINDOW_CHROME_H

#include <QObject>
#include <QPointF>

class QWindow;

class WindowChrome : public QObject {
    Q_OBJECT

public:
    explicit WindowChrome(QObject* parent = nullptr);

    Q_INVOKABLE bool isMaximized(QWindow* window) const;
    Q_INVOKABLE void showSystemMenu(QWindow* window, const QPointF& globalPosition);
};

#endif // REARK_WINDOW_CHROME_H

#ifndef FOCUSASSISTANT_H
#define FOCUSASSISTANT_H

#include <QWidget>

namespace Ui {
class FocusAssistant;
}

class FocusAssistant : public QWidget
{
    Q_OBJECT

public:
    explicit FocusAssistant(QWidget *parent = 0);
    ~FocusAssistant();

    bool displayLeft;
    void playImage(QPixmap img);

private:
    Ui::FocusAssistant *ui;
    bool checkstate();
signals:
    void winhide();
private slots:
    void checkchange();
};

#endif // FOCUSASSISTANT_H

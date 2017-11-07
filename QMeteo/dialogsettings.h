#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>
#include <QFile>
#include <QDir>
#include <QTextStream>

#include "config.h"

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(QWidget *parent = 0);
    ~DialogSettings();

private slots:
    void on_btnClose_clicked();

    void on_btnApply_clicked();

private:
    Ui::DialogSettings *ui;
};

#endif // DIALOGSETTINGS_H

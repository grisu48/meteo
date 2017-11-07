#include "dialogsettings.h"
#include "ui_dialogsettings.h"

DialogSettings::DialogSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings)
{
    ui->setupUi(this);

    // Load configuration
    // Load default settings
    Config config(QDir::homePath() + "/.qmeteo.cf");
    ui->txtRemote->setText(config.get("remoteHost", "localhost:8900").trimmed());

    long l_interval = 0L;
    QString interval = config.get("refreshInterval", "").trimmed();
    if(!interval.isEmpty()) {
        bool ok = false;
        l_interval = interval.toLong(&ok);
        if(ok)
            ui->txtRefreshInterval->setText(QString::number(l_interval));
    }

    ui->chkAutoconnect->setChecked(config.get("autoconnect", "0") == "1");
}

DialogSettings::~DialogSettings()
{
    delete ui;
}

void DialogSettings::on_btnClose_clicked()
{
    this->close();
}

void DialogSettings::on_btnApply_clicked()
{
    QString remote = ui->txtRemote->text().trimmed();
    QString interval = ui->txtRefreshInterval->text().trimmed();
    long l_interval = 0L;

    bool hasInterval = !interval.isEmpty();
    if(hasInterval)
        l_interval = interval.toLong(&hasInterval);


    QString filename = QDir::homePath() + "/.qmeteo.cf";
    QFile file(filename);
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        if(!remote.isEmpty())
            stream << "remoteHost = " << remote << "\n";
        if(ui->chkAutoconnect->isChecked())
            stream << "autoconnect = 1\n";
        else
            stream << "autoconnect = 0\n";
        if(hasInterval)
            stream << "refreshInterval = " << QString::number(l_interval) << "\n";
    }
}

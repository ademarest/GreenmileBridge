#include "as400inputwidget.h"
#include "ui_as400inputwidget.h"

AS400InputWidget::AS400InputWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AS400InputWidget)
{
    ui->setupUi(this);

    connect(ui->as400CommitSettings, &QPushButton::pressed, this, &AS400InputWidget::saveSettings);
    AS400Settings_ = settings_.loadSettings(QFile(dbPath_), AS400Settings_);
    loadSettings();
}

AS400InputWidget::~AS400InputWidget()
{
    delete ui;
}

void AS400InputWidget::saveSettings()
{
    AS400Settings_["password"] = ui->as400PasswordLineEdit->text();
    AS400Settings_["username"] = ui->as400UsernameLineEdit->text();
    AS400Settings_["driver"] = ui->as400DriverLineEdit->text().simplified();
    AS400Settings_["system"] = ui->as400SystemLineEdit->text().simplified();

    settings_.saveSettings(QFile(dbPath_), AS400Settings_);
}

void AS400InputWidget::loadSettings()
{
    AS400Settings_ = settings_.loadSettings(QFile(dbPath_), AS400Settings_);
    ui->as400PasswordLineEdit->setText(AS400Settings_["password"].toString());
    ui->as400UsernameLineEdit->setText(AS400Settings_["username"].toString());
    ui->as400DriverLineEdit->setText(AS400Settings_["driver"].toString());
    ui->as400SystemLineEdit->setText(AS400Settings_["system"].toString());
}

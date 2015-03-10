#include "focusassistant.h"
#include "ui_focusassistant.h"

FocusAssistant::FocusAssistant(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FocusAssistant)
{
    ui->setupUi(this);

    displayLeft = checkstate();
    connect(ui->leftCamera, SIGNAL(toggled(bool)), this, SLOT(checkchange()));
    connect(ui->okButton, SIGNAL(clicked()), SIGNAL(winhide()));
    connect(ui->okButton, SIGNAL(clicked()), this, SLOT(hide()));
    //connect(this, SIGNAL(destroyed()), SIGNAL(winhide()));
}

FocusAssistant::~FocusAssistant()
{
    delete ui;
}

bool FocusAssistant::checkstate()
{
    return (ui->leftCamera->isChecked())?(true):(false);
}

void FocusAssistant::checkchange()
{
    displayLeft = checkstate();
}

void FocusAssistant::playImage(QPixmap img)
{
    ui->imageDisplay->setPixmap(img);
}

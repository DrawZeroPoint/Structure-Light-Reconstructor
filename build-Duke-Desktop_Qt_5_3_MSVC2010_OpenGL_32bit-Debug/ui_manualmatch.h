/********************************************************************************
** Form generated from reading UI file 'manualmatch.ui'
**
** Created by: Qt User Interface Compiler version 5.3.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MANUALMATCH_H
#define UI_MANUALMATCH_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ManualMatch
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *leftImage;
    QLabel *rightImage;
    QHBoxLayout *horizontalLayout_4;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QLabel *current;
    QLabel *label;
    QLineEdit *idEdit;
    QPushButton *confirmButton;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *finishButton;
    QPushButton *resetButton;
    QPushButton *cancelButton;

    void setupUi(QWidget *ManualMatch)
    {
        if (ManualMatch->objectName().isEmpty())
            ManualMatch->setObjectName(QStringLiteral("ManualMatch"));
        ManualMatch->resize(815, 575);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ManualMatch->sizePolicy().hasHeightForWidth());
        ManualMatch->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QStringLiteral(":/splash.png"), QSize(), QIcon::Normal, QIcon::Off);
        ManualMatch->setWindowIcon(icon);
        ManualMatch->setAutoFillBackground(true);
        gridLayout = new QGridLayout(ManualMatch);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(15);
        gridLayout->setContentsMargins(6, 10, 6, 10);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(20);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        leftImage = new QLabel(ManualMatch);
        leftImage->setObjectName(QStringLiteral("leftImage"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(leftImage->sizePolicy().hasHeightForWidth());
        leftImage->setSizePolicy(sizePolicy1);
        leftImage->setMaximumSize(QSize(640, 512));
        leftImage->setFrameShape(QFrame::Box);
        leftImage->setScaledContents(true);

        horizontalLayout->addWidget(leftImage);

        rightImage = new QLabel(ManualMatch);
        rightImage->setObjectName(QStringLiteral("rightImage"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(rightImage->sizePolicy().hasHeightForWidth());
        rightImage->setSizePolicy(sizePolicy2);
        rightImage->setMaximumSize(QSize(640, 512));
        rightImage->setFrameShape(QFrame::Box);
        rightImage->setScaledContents(true);

        horizontalLayout->addWidget(rightImage);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(100, -1, 100, -1);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(10);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label_2 = new QLabel(ManualMatch);
        label_2->setObjectName(QStringLiteral("label_2"));
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(label_2);

        current = new QLabel(ManualMatch);
        current->setObjectName(QStringLiteral("current"));
        sizePolicy.setHeightForWidth(current->sizePolicy().hasHeightForWidth());
        current->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(current);

        label = new QLabel(ManualMatch);
        label->setObjectName(QStringLiteral("label"));
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        horizontalLayout_2->addWidget(label);

        idEdit = new QLineEdit(ManualMatch);
        idEdit->setObjectName(QStringLiteral("idEdit"));
        QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(idEdit->sizePolicy().hasHeightForWidth());
        idEdit->setSizePolicy(sizePolicy3);
        idEdit->setMaximumSize(QSize(60, 16777215));
        idEdit->setClearButtonEnabled(false);

        horizontalLayout_2->addWidget(idEdit);

        confirmButton = new QPushButton(ManualMatch);
        confirmButton->setObjectName(QStringLiteral("confirmButton"));

        horizontalLayout_2->addWidget(confirmButton);


        horizontalLayout_4->addLayout(horizontalLayout_2);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(10);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        finishButton = new QPushButton(ManualMatch);
        finishButton->setObjectName(QStringLiteral("finishButton"));

        horizontalLayout_3->addWidget(finishButton);

        resetButton = new QPushButton(ManualMatch);
        resetButton->setObjectName(QStringLiteral("resetButton"));

        horizontalLayout_3->addWidget(resetButton);

        cancelButton = new QPushButton(ManualMatch);
        cancelButton->setObjectName(QStringLiteral("cancelButton"));

        horizontalLayout_3->addWidget(cancelButton);


        horizontalLayout_4->addLayout(horizontalLayout_3);


        gridLayout->addLayout(horizontalLayout_4, 1, 0, 1, 1);


        retranslateUi(ManualMatch);

        QMetaObject::connectSlotsByName(ManualMatch);
    } // setupUi

    void retranslateUi(QWidget *ManualMatch)
    {
        ManualMatch->setWindowTitle(QApplication::translate("ManualMatch", "Manual Match Assistant", 0));
        leftImage->setText(QApplication::translate("ManualMatch", "Left Image", 0));
        rightImage->setText(QApplication::translate("ManualMatch", "Right Image", 0));
        label_2->setText(QApplication::translate("ManualMatch", "<html><head/><body><p>Current Point Num:</p></body></html>", 0));
        current->setText(QApplication::translate("ManualMatch", "0", 0));
        label->setText(QApplication::translate("ManualMatch", "Set ID", 0));
        confirmButton->setText(QApplication::translate("ManualMatch", "Confirm", 0));
        finishButton->setText(QApplication::translate("ManualMatch", "Finish", 0));
        resetButton->setText(QApplication::translate("ManualMatch", "Reset", 0));
        cancelButton->setText(QApplication::translate("ManualMatch", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class ManualMatch: public Ui_ManualMatch {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MANUALMATCH_H

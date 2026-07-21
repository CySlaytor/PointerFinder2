#include "AboutDialog.h"

#include "../../Models/GlobalSettings.h"
#include "../Styles/ThemeManager.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>

#ifndef VERSION_STRING
#define VERSION_STRING "0.0.0"
#endif

namespace PointerFinder2::UI {

    // Sets up the visual structure of the Credits window, loading the app icon, 
    // the system information, and contributor credits blocks.
    AboutDialog::AboutDialog(QWidget* parent)
        : QDialog(parent) {
        setWindowTitle("About Pointer Finder 2.0");
        setModal(true);
        setFixedSize(520, 430);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        setupLayout();
    }

    AboutDialog::~AboutDialog() = default;

    void AboutDialog::setupLayout() {
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(15, 15, 15, 15);
        mainLayout->setSpacing(12);

        auto* topLayout = new QHBoxLayout();
        topLayout->setSpacing(15);

        m_iconLabel = new QLabel(this);
        QIcon appIcon = ThemeManager::getIcon(":/appicon.ico", DataModels::GlobalSettings::activeTheme);
        if (!appIcon.isNull()) {
            m_iconLabel->setPixmap(appIcon.pixmap(QSize(64, 64)));
        }
        m_iconLabel->setAlignment(Qt::AlignTop);
        topLayout->addWidget(m_iconLabel);

        auto* titleContainer = new QVBoxLayout();
        titleContainer->setSpacing(2);

        m_titleLabel = new QLabel("Pointer Finder 2.0", this);
        QFont titleFont = m_titleLabel->font();
        titleFont.setPointSize(14);
        titleFont.setBold(true);
        m_titleLabel->setFont(titleFont);
        titleContainer->addWidget(m_titleLabel);

        QLabel* versionLabel = new QLabel(QString("Version %1 (Qt Port)").arg(VERSION_STRING), this);
        QFont verFont = versionLabel->font();
        verFont.setBold(true);
        versionLabel->setFont(verFont);
        titleContainer->addWidget(versionLabel);

        titleContainer->addStretch();
        topLayout->addLayout(titleContainer, 1);
        mainLayout->addLayout(topLayout);

        m_descLabel = new QLabel(this);
        m_descLabel->setWordWrap(true);
        m_descLabel->setTextFormat(Qt::RichText);
        m_descLabel->setText(
            "<p style='line-height: 120%;'>PointerFinder2 is a pointer scanning utility designed to discover complex "
            "pointer chains without requiring manual debugger analysis. Originally developed in C#, it was later "
            "rewritten in Qt to provide a modern, portable foundation aimed at supporting cross-platform structures in the future.</p>"
            "<p style='line-height: 120%;'>This port serves as an architectural proof of concept, demonstrating how other "
            "RetroAchievements toolchains (such as RALibretro and RA_Integration) may transition to cross-platform Qt "
            "framework targets to provide native support for Linux and macOS down the line.</p>"
        );
        mainLayout->addWidget(m_descLabel);

        auto* divider = new QFrame(this);
        divider->setFrameShape(QFrame::HLine);
        divider->setFrameShadow(QFrame::Sunken);
        mainLayout->addWidget(divider);

        m_creditsLabel = new QLabel(this);
        m_creditsLabel->setWordWrap(true);
        m_creditsLabel->setTextFormat(Qt::RichText);
        m_creditsLabel->setOpenExternalLinks(true);
        m_creditsLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_creditsLabel->setText(
            "<b>Project Contributors:</b>"
            "<table cellspacing='4' cellpadding='0' style='margin-top: 5px;'>"
            "<tr>"
            "<td valign='top' style='width: 80px;'><b>Creator:</b></td>"
            "<td valign='top'><a href='https://retroachievements.org/user/CySlaytor'>CySlaytor</a> &mdash; Core concept, logic architecture, and tool design. Developed to replace tedious debugger inspection workflows.</td>"
            "</tr>"
            "<tr>"
            "<td valign='top'><b>Contributor:</b></td>"
            "<td valign='top'><a href='https://retroachievements.org/user/Homuki'>Homuki</a> &mdash; Assisted with validation, design concept parameters, and initial verification stages during the early C# codebase development.</td>"
            "</tr>"
            "<tr>"
            "<td valign='top'><b>Artwork:</b></td>"
            "<td valign='top'><a href='https://retroachievements.org/user/ZeeRA'>ZeeRA</a> &mdash; Original design of the application's graphic icon (the 2.0 magnifying glass logo).</td>"
            "</tr>"
            "</table>"
        );
        mainLayout->addWidget(m_creditsLabel, 1);

        auto* buttonLayout = new QHBoxLayout();
        m_closeButton = new QPushButton("Close", this);
        m_closeButton->setObjectName("closeButton");
        m_closeButton->setFixedWidth(75);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_closeButton);
        mainLayout->addLayout(buttonLayout);

        connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
    }

}

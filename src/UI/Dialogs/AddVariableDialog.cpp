#include "AddVariableDialog.h"

#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

namespace PointerFinder2::UI {

AddVariableDialog::AddVariableDialog(const QString& defaultType, const QString& defaultName, QWidget* parent) : QDialog(parent) {
    setWindowTitle("Add Variable");
    setModal(true);
    setFixedSize(365, 104);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setupLayout(defaultType, defaultName);
    loadPositionSettings();
}

void AddVariableDialog::loadPositionSettings() {
    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DialogPositions");
    if (settings.contains("AddVariableDialog")) {
        QString val = settings.value("AddVariableDialog").toString();
        if (!val.startsWith("@ByteArray")) {
            QStringList parts = val.split(',');
            if (parts.size() >= 2) {
                bool okX = false, okY = false;
                int x = parts[0].trimmed().toInt(&okX);
                int y = parts[1].trimmed().toInt(&okY);
                if (okX && okY) {
                    move(x, y);
                }
            }
        }
    }
    settings.endGroup();
}

void AddVariableDialog::savePositionSettings() {
    QString iniPath = QDir(QCoreApplication::applicationDirPath()).filePath("settings.ini");
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.beginGroup("DialogPositions");
    settings.setValue("AddVariableDialog", QString("%1, %2").arg(pos().x()).arg(pos().y()));
    settings.endGroup();
}

void AddVariableDialog::done(int r) {
    savePositionSettings();
    QDialog::done(r);
}

void AddVariableDialog::setupLayout(const QString& defaultType, const QString& defaultName) {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Side-by-side columns: Compact Data Type on the left, Variable Name on the right
    auto* fieldsLayout = new QHBoxLayout();
    fieldsLayout->setSpacing(8);

    // Column 1: Data Type
    auto* typeColLayout = new QVBoxLayout();
    typeColLayout->setSpacing(3);

    auto* typeLabel = new QLabel("Data Type:", this);
    m_typeCombo = new QComboBox(this);
    m_typeCombo->setFixedWidth(110);
    // Standard data types >= 8-bit matching RetroAchievements memory allocation standards
    m_typeCombo->addItems({"8-bit", "16-bit", "24-bit", "32-bit", "16-bit BE", "24-bit BE", "32-bit BE", "Float", "Float BE", "Double32", "Double32 BE", "MBF32", "MBF32 LE"});

    int typeIdx = m_typeCombo->findText(defaultType, Qt::MatchFixedString);
    if (typeIdx != -1) {
        m_typeCombo->setCurrentIndex(typeIdx);
    } else {
        for (int i = 0; i < m_typeCombo->count(); ++i) {
            if (m_typeCombo->itemText(i).compare(defaultType, Qt::CaseInsensitive) == 0) {
                m_typeCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    typeColLayout->addWidget(typeLabel);
    typeColLayout->addWidget(m_typeCombo);
    fieldsLayout->addLayout(typeColLayout);

    // Column 2: Variable Name
    auto* nameColLayout = new QVBoxLayout();
    nameColLayout->setSpacing(3);

    auto* nameLabel = new QLabel("Variable Name:", this);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setText(defaultName);
    m_nameEdit->setPlaceholderText("e.g. Health, Score");

    nameColLayout->addWidget(nameLabel);
    nameColLayout->addWidget(m_nameEdit);
    fieldsLayout->addLayout(nameColLayout, 1);

    mainLayout->addLayout(fieldsLayout);

    // Bottom Action Buttons
    auto* buttonLayout = new QHBoxLayout();
    m_okButton = new QPushButton("OK", this);
    m_okButton->setObjectName("okButton");
    m_okButton->setDefault(true);

    m_cancelButton = new QPushButton("Cancel", this);
    m_cancelButton->setObjectName("cancelButton");

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::returnPressed, this, &QDialog::accept);

    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

QString AddVariableDialog::getDataType() const { return m_typeCombo ? m_typeCombo->currentText() : "32-bit"; }

QString AddVariableDialog::getVariableName() const { return m_nameEdit ? m_nameEdit->text().trimmed() : QString(); }

QString AddVariableDialog::getFullDescription() const {
    QString type = getDataType();
    QString name = getVariableName();
    if (name.isEmpty()) {
        name = "Variable";
    }

    if (name.startsWith(QString("[%1]").arg(type), Qt::CaseInsensitive)) {
        return name;
    }

    return QString("[%1] %2").arg(type, name);
}

} // namespace PointerFinder2::UI

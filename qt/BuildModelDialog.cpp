#include "BuildModelDialog.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDialogButtonBox>

BuildModelDialog::BuildModelDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Build Model"));

    _fontCombo = new QComboBox(this);
    _fontCombo->addItem(tr("7-Segment"),      (int)FontStyle::Seg7);
    _fontCombo->addItem(tr("7-Segment Thin"), (int)FontStyle::ThinSeg7);
    _fontCombo->addItem(tr("Dot Matrix"),     (int)FontStyle::Pixel);
    _fontCombo->addItem(tr("Blank"),          (int)FontStyle::Blank);

    _depthSpin = new QDoubleSpinBox(this);
    _depthSpin->setRange(0.01, 10.0);
    _depthSpin->setValue(0.5);
    _depthSpin->setDecimals(2);
    _depthSpin->setSuffix(tr(" mm"));

    _angleSpin = new QDoubleSpinBox(this);
    _angleSpin->setRange(0.0, 10.0);
    _angleSpin->setValue(1.0);
    _angleSpin->setDecimals(1);
    _angleSpin->setSuffix(tr(" °"));

    _pathEdit = new QLineEdit(this);
    _pathEdit->setPlaceholderText(tr("Output file path…"));

    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);

    _okBtn = new QPushButton(tr("Build Model"), this);
    _okBtn->setEnabled(false);
    _okBtn->setDefault(true);
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), this);

    // Layout
    QFormLayout* form = new QFormLayout;
    form->addRow(tr("Font:"),          _fontCombo);
    form->addRow(tr("Engrave depth:"), _depthSpin);
    form->addRow(tr("Draft angle:"),   _angleSpin);

    QHBoxLayout* pathRow = new QHBoxLayout;
    pathRow->addWidget(_pathEdit);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("Output file:"), pathRow);

    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->addStretch();
    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(_okBtn);

    QVBoxLayout* main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addLayout(btnRow);

    connect(browseBtn, &QPushButton::clicked, this, &BuildModelDialog::browse);
    connect(_pathEdit, &QLineEdit::textChanged, this, &BuildModelDialog::updateOkState);
    connect(_okBtn,    &QPushButton::clicked,   this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked,   this, &QDialog::reject);
}

void BuildModelDialog::browse()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save STL File"), _pathEdit->text(), tr("STL Files (*.stl)"));
    if (!path.isEmpty()) {
        if (!path.endsWith(".stl", Qt::CaseInsensitive))
            path += ".stl";
        _pathEdit->setText(path);
    }
}

void BuildModelDialog::updateOkState()
{
    _okBtn->setEnabled(!_pathEdit->text().trimmed().isEmpty());
}

FontStyle BuildModelDialog::selectedFont() const
{
    return (FontStyle)_fontCombo->currentData().toInt();
}

double BuildModelDialog::engraveDepth() const
{
    return _depthSpin->value();
}

double BuildModelDialog::draftAngleDeg() const
{
    return _angleSpin->value();
}

QString BuildModelDialog::filePath() const
{
    QString p = _pathEdit->text().trimmed();
    if (!p.isEmpty() && !p.endsWith(".stl", Qt::CaseInsensitive))
        p += ".stl";
    return p;
}

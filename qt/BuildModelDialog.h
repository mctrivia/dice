#pragma once
#include <QDialog>
#include "stl/Font.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;

// Shown before every STL export.  Collects font, engrave depth, draft angle,
// and output file path in one place.
class BuildModelDialog : public QDialog {
    Q_OBJECT
public:
    explicit BuildModelDialog(QWidget* parent = nullptr);

    std::unique_ptr<Font> selectedFont() const;
    double     engraveDepth()  const;   // millimetres
    double     draftAngleDeg() const;   // degrees
    QString    filePath()      const;

private slots:
    void browse();
    void updateOkState();

private:
    QComboBox*      _fontCombo;
    QDoubleSpinBox* _depthSpin;
    QDoubleSpinBox* _angleSpin;
    QLineEdit*      _pathEdit;
    QPushButton*    _okBtn;
};

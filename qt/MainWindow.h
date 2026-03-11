#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <array>
#include "Die.h"
#include "OptimizationThread.h"

class DieVisualization;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(std::array<Die*, THREAD_COUNT>& dieArray,
                        QWidget* parent = nullptr);

signals:
    void startRequested(unsigned int sides);

private slots:
    void onStartButtonClicked();
    void onSideCountChanged(const QString& text);

private:
    QWidget*          _inputBar;
    QLineEdit*        _sideCountInput;
    QPushButton*      _startButton;
    DieVisualization* _vis;
    QPushButton*      _pointsButton;
    QPushButton*      _buildModelButton;
};

// PointsWindow.h
#ifndef POINTSWINDOW_H
#define POINTSWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <array>
#include <QPushButton>
#include "Die.h"
#include "OptimizationThread.h"

class PointsWindow : public QWidget {
Q_OBJECT
public:
    PointsWindow(double radius, std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent = nullptr);

public slots:
    void updateTable();

private:
    QDoubleSpinBox* faceToCenterSpinBox;
    QDoubleSpinBox* radiusDoubleBox;
    QTableWidget* pointsTable;
    QPushButton* buildModelButton;
    Die* bestDie;
    double maxRadius() const;
    void buildModel();
    void updateRadiusRange();

    void populateTable();
};

#endif // POINTSWINDOW_H
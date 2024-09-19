#include "PointsWindow.h"
#include <QFileDialog>
#include <limits>
#include "STL.h"
#include <QHeaderView>


// PointsWindow.cpp
PointsWindow::PointsWindow(double radius, std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent)
        : QWidget(parent) {
    //Compute Best
    size_t bestIndex = 0;
    double bestStress = std::numeric_limits<double>::max();
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (dieArray[i] != nullptr) {
            double stress = dieArray[i]->getBest().getTotalStress();
            if (stress < bestStress) {
                bestStress = stress;
                bestIndex = i;
            }
        }
    }
    bestDie = dieArray[bestIndex];


    // Create the radius input
    faceToCenterSpinBox = new QDoubleSpinBox(this);
    faceToCenterSpinBox->setRange(0.0, 1000.0);
    faceToCenterSpinBox->setValue(radius);
    faceToCenterSpinBox->setDecimals(4);
    faceToCenterSpinBox->setPrefix("Face to Center Distance: ");

    // Create the secondary radius box
    radiusDoubleBox = new QDoubleSpinBox(this);
    radiusDoubleBox->setDecimals(4);
    radiusDoubleBox->setPrefix("Radius: ");
    radiusDoubleBox->setRange(faceToCenterSpinBox->value(), maxRadius());
    radiusDoubleBox->setValue(faceToCenterSpinBox->value());

    // Create the "Build Model" button
    buildModelButton = new QPushButton("Build Model", this);

    // Create the table
    pointsTable = new QTableWidget(this);
    pointsTable->setColumnCount(4);
    QStringList headers{"Label", "X", "Y", "Z"};
    pointsTable->setHorizontalHeaderLabels(headers);

    // Set headers to stay in place
    pointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    pointsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    pointsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(faceToCenterSpinBox);
    layout->addWidget(radiusDoubleBox);  // Add the secondary radius box to the layout
    layout->addWidget(pointsTable);
    layout->addWidget(buildModelButton);  // Add the Build Model button to the layout
    layout->setStretch(0, 0); // Radius input takes minimal space
    layout->setStretch(1, 1); // Table takes the rest

    setLayout(layout);

    // Set window size to 2/3 of parent width and full height
    if (parent) {
        int parentWidth = parent->width();
        int parentHeight = parent->height() - 30;
        resize(parentWidth * 2 / 3, parentHeight);
    }

    // Connect signals and slots
    connect(faceToCenterSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PointsWindow::updateRadiusRange);
    connect(buildModelButton, &QPushButton::clicked, this, &PointsWindow::buildModel);

    // Initial population of the table
    populateTable();
}

// Update the range for the secondary radius box
void PointsWindow::updateRadiusRange() {
    double cloudRadius = faceToCenterSpinBox->value();
    double max=maxRadius();
    double radius = radiusDoubleBox->value();
    radiusDoubleBox->setRange(cloudRadius, max);
    if ((radius < cloudRadius) || (radius > max)) {
        radius= ((max - cloudRadius) / 2) + cloudRadius;//set to middle of range
    }
    radiusDoubleBox->setValue(radius); // Set to the updated radius as default
    populateTable();
}

// Build model function to generate STL file
void PointsWindow::buildModel() {
    double cloudRadius = faceToCenterSpinBox->value();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save STL File"), "", tr("STL Files (*.stl)"));
    if (!fileName.isEmpty()) {
        std::vector<Vec3> points;
        for (size_t i = 0; i < bestDie->getBest().sideCount(); ++i) {
            Vec3 point= bestDie->getBest().getPoint(i) * cloudRadius;
            points.push_back(point);
        }

        // Run createSTL with the chosen radius from the secondary radius box
        createSTL(radiusDoubleBox->value(), points, fileName.toStdString());
    }
}

// Example stub function for computeMaxRadius (you'll replace this with actual logic)
double PointsWindow::maxRadius() const {
    double cloudRadius = faceToCenterSpinBox->value();
    std::vector<Vec3> points;
    for (size_t i = 0; i < bestDie->getBest().sideCount(); ++i) {
        Vec3 point= bestDie->getBest().getPoint(i) * cloudRadius;
        points.push_back(point);
    }
    return computeMaxRadius(points);
}

void PointsWindow::populateTable() {
    double cloudRadius = faceToCenterSpinBox->value();

    // Clear existing data
    pointsTable->setRowCount(0);

    // Populate the table with points data
    auto labels = bestDie->getLabels();
    for (size_t i = 0; i < bestDie->getBest().sideCount(); ++i) {
        const auto& point = bestDie->getBest().getPoint(i);

        int row = pointsTable->rowCount();
        pointsTable->insertRow(row);

        //label
        QTableWidgetItem* lItem = new QTableWidgetItem(QString::number(labels[i], 10));
        pointsTable->setItem(row, 0, lItem);

        // X coordinate
        QTableWidgetItem* xItem = new QTableWidgetItem(QString::number(point.x * cloudRadius, 'f', 6));
        pointsTable->setItem(row, 1, xItem);

        // Y coordinate
        QTableWidgetItem* yItem = new QTableWidgetItem(QString::number(point.y * cloudRadius, 'f', 6));
        pointsTable->setItem(row, 2, yItem);

        // Z coordinate
        QTableWidgetItem* zItem = new QTableWidgetItem(QString::number(point.z * cloudRadius, 'f', 6));
        pointsTable->setItem(row, 3, zItem);
    }
}

void PointsWindow::updateTable() {
    populateTable();
}

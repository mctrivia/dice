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
    _bestDie = dieArray[bestIndex];


    // Create the radius input
    _faceToCenterSpinBox = new QDoubleSpinBox(this);
    _faceToCenterSpinBox->setRange(0.0, 1000.0);
    _faceToCenterSpinBox->setValue(radius);
    _faceToCenterSpinBox->setDecimals(4);
    _faceToCenterSpinBox->setPrefix("Face to Center Distance: ");

    // Create the secondary radius box
    _radiusDoubleBox = new QDoubleSpinBox(this);
    _radiusDoubleBox->setDecimals(4);
    _radiusDoubleBox->setPrefix("Radius: ");
    _radiusDoubleBox->setRange(_faceToCenterSpinBox->value(), maxRadius());
    _radiusDoubleBox->setValue(_faceToCenterSpinBox->value());

    // Create the "Build Model" button
    _buildModelButton = new QPushButton("Build Model", this);

    // Create the table
    _pointsTable = new QTableWidget(this);
    _pointsTable->setColumnCount(4);
    QStringList headers{"Label", "X", "Y", "Z"};
    _pointsTable->setHorizontalHeaderLabels(headers);

    // Set headers to stay in place
    _pointsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    _pointsTable->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _pointsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(_faceToCenterSpinBox);
    layout->addWidget(_radiusDoubleBox);  // Add the secondary radius box to the layout
    layout->addWidget(_pointsTable);
    layout->addWidget(_buildModelButton);  // Add the Build Model button to the layout
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
    connect(_faceToCenterSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &PointsWindow::updateRadiusRange);
    connect(_buildModelButton, &QPushButton::clicked, this, &PointsWindow::buildModel);

    // Initial population of the table
    populateTable();
}

// Update the range for the secondary radius box
void PointsWindow::updateRadiusRange() {
    double cloudRadius = _faceToCenterSpinBox->value();
    double max = maxRadius();
    double radius = _radiusDoubleBox->value();
    _radiusDoubleBox->setRange(cloudRadius, max);
    if ((radius < cloudRadius) || (radius > max)) {
        radius = ((max - cloudRadius) / 2) + cloudRadius;//set to middle of range
    }
    _radiusDoubleBox->setValue(radius); // Set to the updated radius as default
    populateTable();
}

// Build model function to generate STL file
void PointsWindow::buildModel() {
    double cloudRadius = _faceToCenterSpinBox->value();

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save STL File"), "", tr("STL Files (*.stl)"));
    if (!fileName.isEmpty()) {
        std::vector<Vec3> points;
        for (size_t i = 0; i < _bestDie->getBest().sideCount(); ++i) {
            Vec3 point = _bestDie->getBest().getPoint(i) * cloudRadius;
            points.push_back(point);
        }

        // Run createSTL with the chosen radius from the secondary radius box
        createSTL(_radiusDoubleBox->value(), points, fileName.toStdString());
    }
}

// Example stub function for computeMaxRadius (you'll replace this with actual logic)
double PointsWindow::maxRadius() const {
    double cloudRadius = _faceToCenterSpinBox->value();
    std::vector<Vec3> points;
    for (size_t i = 0; i < _bestDie->getBest().sideCount(); ++i) {
        Vec3 point = _bestDie->getBest().getPoint(i) * cloudRadius;
        points.push_back(point);
    }
    return computeMaxRadius(points);
}

void PointsWindow::populateTable() {
    double cloudRadius = _faceToCenterSpinBox->value();

    // Clear existing data
    _pointsTable->setRowCount(0);

    // Populate the table with points data
    auto labels = _bestDie->getLabels();
    for (size_t i = 0; i < _bestDie->getBest().sideCount(); ++i) {
        const auto& point = _bestDie->getBest().getPoint(i);

        int row = _pointsTable->rowCount();
        _pointsTable->insertRow(row);

        //label
        QTableWidgetItem* lItem = new QTableWidgetItem(QString::number(labels[i], 10));
        _pointsTable->setItem(row, 0, lItem);

        // X coordinate
        QTableWidgetItem* xItem = new QTableWidgetItem(QString::number(point.x * cloudRadius, 'f', 6));
        _pointsTable->setItem(row, 1, xItem);

        // Y coordinate
        QTableWidgetItem* yItem = new QTableWidgetItem(QString::number(point.y * cloudRadius, 'f', 6));
        _pointsTable->setItem(row, 2, yItem);

        // Z coordinate
        QTableWidgetItem* zItem = new QTableWidgetItem(QString::number(point.z * cloudRadius, 'f', 6));
        _pointsTable->setItem(row, 3, zItem);
    }
}

void PointsWindow::updateTable() {
    populateTable();
}

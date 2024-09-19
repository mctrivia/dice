#include "DieVisualization.h"
#include "PointsWindow.h"
#include <QPainter>
#include <QTimer>
#include <QMessageBox>
#include <QCheckBox>

DieVisualization::DieVisualization(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent)
        : QWidget(parent), dieArray(dieArray), pointsWindow(nullptr), optimizationPaused(false), highlightExtremes(true) {
    setMinimumSize(744, 328);

    // Set size policy to allow the layout to use heightForWidth()
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &DieVisualization::updateVisualization);
    timer->start(50); // Update every 50 ms




    // Create the checkbox for "Highlight Extremes"
    highlightCheckbox = new QCheckBox("Highlight Extremes", this);
    highlightCheckbox->setChecked(true);  // Default to checked (true)

    // Position the checkbox above the contribution text in the bottom left
    int checkboxX = 10;  // 10 pixels from left edge
    int checkboxY = rect().height() - 55; // 55 pixels from bottom edge (above the contribution text)
    highlightCheckbox->move(checkboxX, checkboxY);

    // Connect the checkbox state change to the slot
    connect(highlightCheckbox, &QCheckBox::stateChanged, this, &DieVisualization::onHighlightCheckboxChanged);



    // Create the "Points" button
    pointsButton = new QPushButton("Points Show", this);
    pointsButton->setFixedSize(120, 30); // Set button size

    // Position the button and checkbox initially
    int buttonWidth = pointsButton->width();
    int buttonHeight = pointsButton->height();
    int x = width() - buttonWidth - 10; // 10 pixels from right edge
    int y = height() - buttonHeight - 10; // 10 pixels from bottom edge
    pointsButton->move(x, y);

    // Connect the button's clicked signal to a slot
    connect(pointsButton, &QPushButton::clicked, this, &DieVisualization::onPointsButtonClicked);






    // Ensure the window appears in front and is activated
    raise();               // Bring the window to the front
    activateWindow();       // Give the window focus
}

void DieVisualization::updateVisualization() {
    update(); // Trigger a repaint
}

void DieVisualization::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);

    bestMutex.lock();

    // Find the current best die
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

    // Set up painter and font
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 16));
    QFontMetrics fm(painter.font());

    int widgetWidth = rect().width();

    // Display the best die information (Left-aligned)
    QString threadText = QString("Thread: %1").arg(bestIndex + 1);
    int threadTextX = 10;
    int threadTextY = 30;
    painter.drawText(threadTextX, threadTextY, threadText);

    // Display stress level (Centered)
    QString stressText = QString("Stress: %1").arg(QString::number(bestStress, 'f', 6));
    int stressTextWidth = fm.horizontalAdvance(stressText);
    int stressTextX = (widgetWidth - stressTextWidth) / 2; // Center horizontally
    painter.drawText(stressTextX, threadTextY, stressText);

    // Display time since last (Right-aligned)
    QString lastBestText = QString("Last Best: %1s").arg(dieArray[bestIndex]->getSecondsSinceLastBest());
    if (Die::isOptimizationPaused()) lastBestText = QString("Optimization Paused");
    int lastBestTextWidth = fm.horizontalAdvance(lastBestText);
    int lastBestTextX = widgetWidth - lastBestTextWidth - 10; // 10 pixels from right edge
    painter.drawText(lastBestTextX, threadTextY, lastBestText);

    // Display the contribution message (Bottom-left aligned)
    painter.setFont(QFont("Arial", 10));
    QString contributionText = "This app was designed by Matthew Cornelisse.  As part of the usage agreement, 5% of any profits";
    int contributionTextX = 10; // 10 pixels from left edge
    int contributionTextY = rect().height() - 25; // 20 pixels from the bottom edge
    painter.drawText(contributionTextX, contributionTextY, contributionText);
    contributionText = "derived from products designed with the help of this app must be contributed to Matthew Cornelisse.";
    contributionTextY += 15;
    painter.drawText(contributionTextX, contributionTextY, contributionText);

    // Draw the die (consider checkbox value for highlighting extremes)
    if (dieArray[bestIndex] != nullptr) {
        dieArray[bestIndex]->draw(painter, highlightExtremes);
    }

    bestMutex.unlock();
}

void DieVisualization::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Reposition the "Points" button
    int buttonWidth = pointsButton->width();
    int buttonHeight = pointsButton->height();
    int x = width() - buttonWidth - 10; // 10 pixels from right edge
    int y = height() - buttonHeight - 10; // 10 pixels from bottom edge
    pointsButton->move(x, y);

    // Reposition the checkbox
    int checkboxX = 10;
    int checkboxY = rect().height() - 55;
    highlightCheckbox->move(checkboxX, checkboxY);

    // Resize the PointsWindow if it's open
    if (pointsWindow && pointsWindow->isVisible()) {
        int parentWidth = width();
        int parentHeight = height();
        pointsWindow->resize(parentWidth * 2 / 3, parentHeight);
    }
}

void DieVisualization::onPointsButtonClicked() {
    if (pointsWindow && pointsWindow->isVisible()) {
        // Hide the PointsWindow
        pointsWindow->close();
        optimizationPaused = false;
        pointsButton->setText("Points Show");
        Die::resumeOptimization();
    } else {
        // Show the PointsWindow
        if (!pointsWindow) {
            pointsWindow = new PointsWindow(1.0, dieArray, this);
            pointsWindow->setAttribute(Qt::WA_DeleteOnClose);
            connect(pointsWindow, &QWidget::destroyed, this, [this]() {
                pointsWindow = nullptr;
                optimizationPaused = false;
                pointsButton->setText("Points Show");
                Die::resumeOptimization();
            });
        }
        pointsWindow->show();
        optimizationPaused = true;
        pointsButton->setText("Points Hide");
        Die::pauseOptimization();
    }
}

void DieVisualization::onHighlightCheckboxChanged(int state) {
    // Update the highlightExtremes boolean based on the checkbox state
    highlightExtremes = (state == Qt::Checked);
}

bool DieVisualization::hasHeightForWidth() const {
    return true;
}

int DieVisualization::heightForWidth(int width) const {
    return (width + 210) / 3;
}

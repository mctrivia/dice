#include "DieVisualization.h"
#include "PointsWindow.h"
#include <QPainter>
#include <QTimer>
#include <QMessageBox>
#include <QCheckBox>

DieVisualization::DieVisualization(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent)
        : QWidget(parent), _dieArray(dieArray), _pointsWindow(nullptr), _highlightExtremes(true) {
    setMinimumSize(744, 328);

    // Set size policy to allow the layout to use heightForWidth()
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &DieVisualization::updateVisualization);
    _timer->start(50); // Update every 50 ms




    // Create the checkbox for "Highlight Extremes"
    _highlightCheckbox = new QCheckBox("Highlight Extremes", this);
    _highlightCheckbox->setChecked(true);  // Default to checked (true)

    // Position the checkbox above the contribution text in the bottom left
    int checkboxX = 10;  // 10 pixels from left edge
    int checkboxY = rect().height() - 55; // 55 pixels from bottom edge (above the contribution text)
    _highlightCheckbox->move(checkboxX, checkboxY);

    // Connect the checkbox state change to the slot
    connect(_highlightCheckbox, &QCheckBox::stateChanged, this, &DieVisualization::onHighlightCheckboxChanged);



    // Create the "Points" button
    _pointsButton = new QPushButton("Points Show", this);
    _pointsButton->setFixedSize(120, 30); // Set button size

    // Position the button and checkbox initially
    int buttonWidth = _pointsButton->width();
    int buttonHeight = _pointsButton->height();
    int x = width() - buttonWidth - 10; // 10 pixels from right edge
    int y = height() - buttonHeight - 10; // 10 pixels from bottom edge
    _pointsButton->move(x, y);

    // Connect the button's clicked signal to a slot
    connect(_pointsButton, &QPushButton::clicked, this, &DieVisualization::onPointsButtonClicked);






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

    _bestMutex.lock();

    // Find the current best die
    size_t bestIndex = 0;
    double bestStress = std::numeric_limits<double>::max();
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (_dieArray[i] != nullptr) {
            double stress = _dieArray[i]->getBest().getTotalStress();
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
    QString lastBestText = QString("Last Best: %1s").arg(_dieArray[bestIndex]->getSecondsSinceLastBest());
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
    if (_dieArray[bestIndex] != nullptr) {
        _dieArray[bestIndex]->draw(painter, _highlightExtremes);
    }

    _bestMutex.unlock();
}

void DieVisualization::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    // Reposition the "Points" button
    int buttonWidth = _pointsButton->width();
    int buttonHeight = _pointsButton->height();
    int x = width() - buttonWidth - 10; // 10 pixels from right edge
    int y = height() - buttonHeight - 10; // 10 pixels from bottom edge
    _pointsButton->move(x, y);

    // Reposition the checkbox
    int checkboxX = 10;
    int checkboxY = rect().height() - 55;
    _highlightCheckbox->move(checkboxX, checkboxY);

    // Resize the PointsWindow if it's open
    if (_pointsWindow && _pointsWindow->isVisible()) {
        int parentWidth = width();
        int parentHeight = height();
        _pointsWindow->resize(parentWidth * 2 / 3, parentHeight);
    }
}

void DieVisualization::onPointsButtonClicked() {
    if (_pointsWindow && _pointsWindow->isVisible()) {
        // Hide the PointsWindow
        _pointsWindow->close();
        _pointsButton->setText("Points Show");
        Die::resumeOptimization();
    } else {
        // Show the PointsWindow
        if (!_pointsWindow) {
            _pointsWindow = new PointsWindow(1.0, _dieArray, this);
            _pointsWindow->setAttribute(Qt::WA_DeleteOnClose);
            connect(_pointsWindow, &QWidget::destroyed, this, [this]() {
                _pointsWindow = nullptr;
                _pointsButton->setText("Points Show");
                Die::resumeOptimization();
            });
        }
        _pointsWindow->show();
        _pointsButton->setText("Points Hide");
        Die::pauseOptimization();
    }
}

void DieVisualization::onHighlightCheckboxChanged(int state) {
    // Update the highlightExtremes boolean based on the checkbox state
    _highlightExtremes = (state == Qt::Checked);
}

bool DieVisualization::hasHeightForWidth() const {
    return true;
}

int DieVisualization::heightForWidth(int width) const {
    return (width + 210) / 3;
}

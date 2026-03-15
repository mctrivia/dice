#include "DieVisualization.h"
#include "PointsWindow.h"
#include "BuildModelDialog.h"
#include <QPainter>
#include <QTimer>
#include <QMessageBox>
#include "stl/STL.h"

DieVisualization::DieVisualization(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent)
        : QWidget(parent), _dieArray(dieArray), _pointsWindow(nullptr), _highlightExtremes(true) {
    setMinimumSize(744, 328);
    resize(744, 328);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &DieVisualization::updateVisualization);
    _timer->start(50); // Update every 50 ms
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

    // Show placeholder until optimization has started
    if (bestStress == std::numeric_limits<double>::max()) {
        _bestMutex.unlock();
        painter.setPen(Qt::gray);
        painter.setFont(QFont("Arial", 14));
        painter.drawText(rect(), Qt::AlignCenter,
                         "Enter an even number of sides and click Start");
        return;
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
    int contributionTextY = rect().height() - 45; // above the bottom bar
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

    // Resize the PointsWindow if it's open
    if (_pointsWindow && _pointsWindow->isVisible()) {
        int parentWidth = width();
        int parentHeight = height();
        _pointsWindow->resize(parentWidth * 2 / 3, parentHeight);
    }
}

void DieVisualization::togglePoints() {
    if (_pointsWindow && _pointsWindow->isVisible()) {
        _pointsWindow->close();
        Die::resumeOptimization();
        emit pointsWindowToggled(false);
    } else {
        if (!_pointsWindow) {
            _pointsWindow = new PointsWindow(20.0, _dieArray, this);
            _pointsWindow->setAttribute(Qt::WA_DeleteOnClose);
            connect(_pointsWindow, &QWidget::destroyed, this, [this]() {
                _pointsWindow = nullptr;
                Die::resumeOptimization();
                emit pointsWindowToggled(false);
            });
        }
        _pointsWindow->show();
        Die::pauseOptimization();
        emit pointsWindowToggled(true);
    }
}

void DieVisualization::setHighlightExtremes(bool highlight) {
    _highlightExtremes = highlight;
}

void DieVisualization::buildModel() {
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

    BuildModelDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    // Scale to physical mm so engraveDepth (mm) is meaningful.
    const double cloudRadius = 20.0;
    std::vector<Vec3> points;
    for (size_t i = 0; i < _dieArray[bestIndex]->getBest().sideCount(); ++i) {
        Vec3 point = _dieArray[bestIndex]->getBest().getPoint(i) * cloudRadius;
        points.push_back(point);
    }
    double radius = computeMaxRadius(points);
    std::vector<size_t> labels = _dieArray[bestIndex]->getLabels();
    auto font = dlg.selectedFont();
    int limit = font->maxSides();
    if (limit > 0 && (int)labels.size() > limit) {
        QMessageBox::warning(this, tr("Font limit"),
            tr("The selected font supports at most %1 faces. "
               "This die has %2 faces.").arg(limit).arg(labels.size()));
        return;
    }
    createSTL(radius, points, dlg.filePath().toStdString(), labels,
              *font, dlg.engraveDepth(), dlg.draftAngleDeg());
}

bool DieVisualization::hasHeightForWidth() const {
    return true;
}

int DieVisualization::heightForWidth(int width) const {
    return (width + 210) / 3;
}

// DieVisualization.h
#ifndef DIEVISUALIZATION_H
#define DIEVISUALIZATION_H

#include <QWidget>
#include <array>
#include <QMutex>
#include "Die.h"
#include "OptimizationThread.h"
#include "PointsWindow.h"

class DieVisualization : public QWidget {
Q_OBJECT
public:
    explicit DieVisualization(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent = nullptr);

signals:
    void pointsWindowToggled(bool showing);

public slots:
    void togglePoints();
    void setHighlightExtremes(bool highlight);
    void buildModel();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;

private slots:
    void updateVisualization();

private:
    std::array<Die*, THREAD_COUNT>& _dieArray;
    QMutex _bestMutex;
    QTimer* _timer;
    PointsWindow* _pointsWindow;
    bool _highlightExtremes;
};

#endif // DIEVISUALIZATION_H

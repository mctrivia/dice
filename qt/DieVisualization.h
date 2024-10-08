// DieVisualization.h
#ifndef DIEVISUALIZATION_H
#define DIEVISUALIZATION_H

#include <QWidget>
#include <array>
#include <QMutex>
#include <QPushButton>
#include <QCheckBox>
#include "Die.h"
#include "OptimizationThread.h"
#include "PointsWindow.h"

class DieVisualization : public QWidget {
Q_OBJECT
public:
    explicit DieVisualization(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    //QSize sizeHint() const override;
    //QSize minimumSizeHint() const override;
    void resizeEvent(QResizeEvent* event) override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
private slots:
    void updateVisualization();
    void onPointsButtonClicked();
    void onHighlightCheckboxChanged(int state);
private:
    std::array<Die*, THREAD_COUNT>& _dieArray;
    QMutex _bestMutex;
    QTimer* _timer;
    QPushButton* _pointsButton;
    QCheckBox* _highlightCheckbox;

    PointsWindow* _pointsWindow;
    bool _highlightExtremes;
};

#endif // DIEVISUALIZATION_H

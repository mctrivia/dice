#include "MainWindow.h"
#include "DieVisualization.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QCheckBox>

MainWindow::MainWindow(std::array<Die*, THREAD_COUNT>& dieArray, QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("Dice Optimizer");

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Input bar (hidden after Start) ────────────────────────────────────────
    _inputBar = new QWidget();
    QHBoxLayout* inputLayout = new QHBoxLayout(_inputBar);
    inputLayout->setContentsMargins(10, 8, 10, 8);

    inputLayout->addWidget(new QLabel(
        "Optimizes face placement for any even-sided die."
        "  Give Matthew Cornelisse 5% of any profit from products made with this app."));
    inputLayout->addStretch();
    inputLayout->addWidget(new QLabel("Sides:"));

    _sideCountInput = new QLineEdit();
    _sideCountInput->setValidator(new QIntValidator(2, 9998, this));
    _sideCountInput->setFixedWidth(70);
    _sideCountInput->setPlaceholderText("even #");
    inputLayout->addWidget(_sideCountInput);

    _startButton = new QPushButton("Start");
    _startButton->setEnabled(false);
    inputLayout->addWidget(_startButton);

    root->addWidget(_inputBar);

    // ── Visualization (always present, shows placeholder until started) ───────
    _vis = new DieVisualization(dieArray, this);
    root->addWidget(_vis, 1);

    // ── Bottom bar ────────────────────────────────────────────────────────────
    QWidget*     bottomBar    = new QWidget();
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(10, 4, 10, 4);

    QCheckBox* highlightCheckbox = new QCheckBox("Highlight Extremes");
    highlightCheckbox->setChecked(true);
    bottomLayout->addWidget(highlightCheckbox);
    bottomLayout->addStretch();

    _buildModelButton = new QPushButton("Build Model");
    _buildModelButton->setFixedSize(120, 30);
    _buildModelButton->setVisible(false);
    bottomLayout->addWidget(_buildModelButton);

    _pointsButton = new QPushButton("Points Show");
    _pointsButton->setFixedSize(120, 30);
    _pointsButton->setEnabled(false);
    bottomLayout->addWidget(_pointsButton);

    root->addWidget(bottomBar);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(_sideCountInput, &QLineEdit::textChanged,
            this, &MainWindow::onSideCountChanged);
    connect(_sideCountInput, &QLineEdit::returnPressed,
            this, &MainWindow::onStartButtonClicked);
    connect(_startButton, &QPushButton::clicked,
            this, &MainWindow::onStartButtonClicked);

    connect(_buildModelButton, &QPushButton::clicked,
            _vis, &DieVisualization::buildModel);
    connect(_pointsButton, &QPushButton::clicked,
            _vis, &DieVisualization::togglePoints);
    connect(highlightCheckbox, &QCheckBox::stateChanged, _vis,
            [this](int state) {
                _vis->setHighlightExtremes(state == Qt::Checked);
            });
    connect(_vis, &DieVisualization::pointsWindowToggled,
            [this](bool showing) {
                _pointsButton->setText(showing ? "Points Hide" : "Points Show");
                _buildModelButton->setVisible(showing);
            });
}

void MainWindow::onSideCountChanged(const QString& text) {
    bool ok = false;
    int  n  = text.toInt(&ok);
    _startButton->setEnabled(ok && n >= 2 && n % 2 == 0);
}

void MainWindow::onStartButtonClicked() {
    if (!_startButton->isEnabled()) return;
    unsigned int sides = _sideCountInput->text().toUInt();

    // Lock the input so the user can't start twice
    _inputBar->setEnabled(false);
    _pointsButton->setEnabled(true);

    emit startRequested(sides);
}

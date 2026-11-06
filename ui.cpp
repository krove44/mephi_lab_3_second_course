#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QTimer>
#include <QMessageBox>
#include <QFont>
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QCustom3DLabel>
#include <QtDataVisualization/Q3DTheme>

#include <memory>
#include <cmath>
#include <numbers>
#include <sstream>
#include <chrono>
#include "Graph.h"

template <typename T, typename E>
using AvlGraph = Graph<
    avl_tree<std::pair<GraphNode<T>, GraphNode<T>>, GraphEdge<E>>,
    GraphNode<T>,
    GraphEdge<E>
>;

class IGraphController {
public:
    virtual ~IGraphController() = default;
    virtual void addVertex(const std::string& v) = 0;
    virtual void removeVertex(const std::string& v) = 0;
    virtual void addEdge(const std::string& u, const std::string& v, double w) = 0;
    virtual void removeEdge(const std::string& u, const std::string& v) = 0;
    virtual std::vector<std::string> getVertices() = 0;
    virtual std::vector<std::tuple<std::string, std::string, double>> getEdges() = 0;
    virtual std::vector<std::string> bfs(const std::string& start) = 0;
    virtual std::vector<std::string> dfs(const std::string& start) = 0;
    virtual std::vector<std::string> topoSort() = 0;
    virtual bool isDirected() const = 0;
};

template <typename T>
class GraphControllerImpl : public IGraphController {
    AvlGraph<T, double> graph_;

    T parse(const std::string& s) const {
        if constexpr (std::is_same_v<T, int>) {
            return std::stoi(s);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(s);
        } else {
            return s;
        }
    }

    std::string format(const T& val) const {
        if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else {
            std::ostringstream oss;
            oss << val;
            return oss.str();
        }
    }

public:
    explicit GraphControllerImpl(bool directed) : graph_(directed) {}

    bool isDirected() const override { return graph_.is_directed(); }

    void addVertex(const std::string& v) override {
        graph_.add_vertex(parse(v));
    }

    void removeVertex(const std::string& v) override {
        graph_.remove_vertex(parse(v));
    }

    void addEdge(const std::string& u, const std::string& v, double w) override {
        graph_.add_edge(parse(u), parse(v), w);
    }

    void removeEdge(const std::string& u, const std::string& v) override {
        graph_.remove_edge(parse(u), parse(v));
    }

    std::vector<std::string> getVertices() override {
        std::vector<std::string> res;
        for (const auto& v : graph_.get_vertices()) {
            res.push_back(format(v));
        }
        return res;
    }

    std::vector<std::tuple<std::string, std::string, double>> getEdges() override {
        std::vector<std::tuple<std::string, std::string, double>> res;
        auto verts = graph_.get_vertices();
        for (size_t i = 0; i < verts.size(); ++i) {
            for (size_t j = 0; j < verts.size(); ++j) {
                if (!graph_.is_directed() && j < i) continue;
                if (graph_.has_edge(verts[i], verts[j])) {
                    res.emplace_back(format(verts[i]), format(verts[j]), graph_.get_edge(verts[i], verts[j]).value());
                }
            }
        }
        return res;
    }

    std::vector<std::string> bfs(const std::string& start) override {
        std::vector<std::string> res;
        for (const auto& v : graph_.bfs(parse(start))) res.push_back(format(v));
        return res;
    }

    std::vector<std::string> dfs(const std::string& start) override {
        std::vector<std::string> res;
        for (const auto& v : graph_.dfs(parse(start))) res.push_back(format(v));
        return res;
    }

    std::vector<std::string> topoSort() override {
        std::vector<std::string> res;
        for (const auto& v : graph_.topological_sort()) res.push_back(format(v));
        return res;
    }
};

class MainWindow : public QMainWindow {
    Q_OBJECT

private:
    std::unique_ptr<IGraphController> controller_;
    Q3DScatter* scatter_;
    QScatter3DSeries* nodeSeries_;
    QScatter3DSeries* highlightSeries_;
    QScatter3DSeries* edgeSeries_;
    QScatter3DSeries* arrowSeries_;
    std::vector<QCustom3DLabel*> labels_;
    std::vector<std::string> normalNodeNames_;

    QComboBox* typeCombo_;
    QCheckBox* directedCheck_;
    QComboBox* meshCombo_;
    QComboBox* themeCombo_;

    QLineEdit* vertexInput_;
    QLineEdit* fromInput_;
    QLineEdit* toInput_;
    QLineEdit* weightInput_;
    QLineEdit* startNodeInput_;
    QTextEdit* logOutput_;

    QTimer* animTimer_;
    std::vector<std::string> animSequence_;
    size_t animStep_{0};

public:
    MainWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("3D Визуализатор Графов (AVL-Tree Sparse Matrix)");
        resize(1380, 860);

        auto* central = new QWidget(this);
        auto* mainLayout = new QHBoxLayout(central);
        setCentralWidget(central);

        scatter_ = new Q3DScatter();
        QWidget* container = QWidget::createWindowContainer(scatter_);
        container->setMinimumSize(860, 760);

        scatter_->activeTheme()->setType(Q3DTheme::ThemeEbony);
        scatter_->activeTheme()->setLabelBackgroundEnabled(true);
        scatter_->activeTheme()->setFont(QFont("Arial", 12, QFont::Bold));
        // Отключаем тени на стенах, чтобы не было серых пятен рядом с вершинами
        scatter_->setShadowQuality(QAbstract3DGraph::ShadowQualityNone);
        scatter_->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricLeftHigh);

        scatter_->axisX()->setRange(-11.0f, 11.0f);
        scatter_->axisY()->setRange(-11.0f, 11.0f);
        scatter_->axisZ()->setRange(-11.0f, 11.0f);
        scatter_->axisX()->setTitle("X");
        scatter_->axisY()->setTitle("Y");
        scatter_->axisZ()->setTitle("Z");

        // Основные вершины (голубые сферы)
        nodeSeries_ = new QScatter3DSeries();
        nodeSeries_->setItemSize(0.26f);
        nodeSeries_->setBaseColor(QColor(0, 185, 255));
        nodeSeries_->setMesh(QAbstract3DSeries::MeshSphere);
        nodeSeries_->setItemLabelFormat(QStringLiteral("Вершина @userAdded"));

        // Подсвеченные при обходе вершины (золотые сферы)
        highlightSeries_ = new QScatter3DSeries();
        highlightSeries_->setItemSize(0.32f);
        highlightSeries_->setBaseColor(QColor(255, 200, 0));
        highlightSeries_->setMesh(QAbstract3DSeries::MeshSphere);
        highlightSeries_->setItemLabelFormat(QStringLiteral("Посещена"));

        // Тело ребра (гладкая линия из маленьких сфер)
        edgeSeries_ = new QScatter3DSeries();
        edgeSeries_->setItemSize(0.065f);
        edgeSeries_->setBaseColor(QColor(130, 165, 195));
        edgeSeries_->setMesh(QAbstract3DSeries::MeshSphere);
        edgeSeries_->setItemLabelVisible(false);

        // Наконечник направленного ребра (оранжево-красный сегмент у входа в целевую вершину V)
        arrowSeries_ = new QScatter3DSeries();
        arrowSeries_->setItemSize(0.095f);
        arrowSeries_->setBaseColor(QColor(255, 75, 50));
        arrowSeries_->setMesh(QAbstract3DSeries::MeshSphere);
        arrowSeries_->setItemLabelVisible(false);

        scatter_->addSeries(nodeSeries_);
        scatter_->addSeries(highlightSeries_);
        scatter_->addSeries(edgeSeries_);
        scatter_->addSeries(arrowSeries_);

        connect(nodeSeries_, &QScatter3DSeries::selectedItemChanged, this, &MainWindow::onNodeClicked);

        auto* panel = new QVBoxLayout();
        panel->setSpacing(10);

        // 1. Настройки типа графа и 3D вида
        auto* configGroup = new QGroupBox("Конфигурация графа и 3D-сцены");
        auto* configLayout = new QVBoxLayout(configGroup);

        typeCombo_ = new QComboBox();
        typeCombo_->addItems({"int (Целые числа)", "std::string (Строки)", "double (Вещественные)"});
        directedCheck_ = new QCheckBox("Ориентированный граф");
        directedCheck_->setChecked(true);
        auto* resetBtn = new QPushButton("Пересоздать пустой граф");
        auto* presetBtn = new QPushButton("Сгенерировать демо DAG-граф");

        meshCombo_ = new QComboBox();
        meshCombo_->addItems({"Сфера (Sphere)", "Куб (Cube)", "Пирамида (Pyramid)", "Цилиндр (Cylinder)"});

        themeCombo_ = new QComboBox();
        themeCombo_->addItems({"Ebony (Темная контрастная)", "Qt (Светлая)", "Retro", "Primary Colors"});

        configLayout->addWidget(new QLabel("Тип данных вершин (T):"));
        configLayout->addWidget(typeCombo_);
        configLayout->addWidget(directedCheck_);
        configLayout->addWidget(resetBtn);
        configLayout->addWidget(presetBtn);
        configLayout->addWidget(new QLabel("3D-форма вершин:"));
        configLayout->addWidget(meshCombo_);
        configLayout->addWidget(new QLabel("Тема 3D-сцены:"));
        configLayout->addWidget(themeCombo_);
        panel->addWidget(configGroup);

        // 2. Управление вершинами
        auto* vertGroup = new QGroupBox("Управление вершинами");
        auto* vertLayout = new QHBoxLayout(vertGroup);
        vertexInput_ = new QLineEdit();
        vertexInput_->setPlaceholderText("Имя вершины...");
        auto* addVertBtn = new QPushButton("+ Вершина");
        auto* remVertBtn = new QPushButton("- Вершина");
        vertLayout->addWidget(vertexInput_);
        vertLayout->addWidget(addVertBtn);
        vertLayout->addWidget(remVertBtn);
        panel->addWidget(vertGroup);

        // 3. Управление ребрами
        auto* edgeGroup = new QGroupBox("Управление ребрами (оранжевый конец = вход в V)");
        auto* edgeLayout = new QVBoxLayout(edgeGroup);
        auto* edgeInputs = new QHBoxLayout();
        fromInput_ = new QLineEdit();
        fromInput_->setPlaceholderText("Откуда (U)");
        toInput_ = new QLineEdit();
        toInput_->setPlaceholderText("Куда (V)");
        weightInput_ = new QLineEdit("1.0");
        weightInput_->setPlaceholderText("Вес");
        edgeInputs->addWidget(fromInput_);
        edgeInputs->addWidget(toInput_);
        edgeInputs->addWidget(weightInput_);

        auto* edgeBtns = new QHBoxLayout();
        auto* addEdgeBtn = new QPushButton("Добавить ребро");
        auto* remEdgeBtn = new QPushButton("Удалить ребро");
        edgeBtns->addWidget(addEdgeBtn);
        edgeBtns->addWidget(remEdgeBtn);

        edgeLayout->addLayout(edgeInputs);
        edgeLayout->addLayout(edgeBtns);
        panel->addWidget(edgeGroup);

        // 4. Алгоритмы и 3D-анимация
        auto* algoGroup = new QGroupBox("Алгоритмы и 3D-анимация");
        auto* algoLayout = new QVBoxLayout(algoGroup);
        startNodeInput_ = new QLineEdit();
        startNodeInput_->setPlaceholderText("Стартовая вершина (для BFS / DFS)");
        auto* bfsBtn = new QPushButton("Запустить BFS (Обход в ширину)");
        auto* dfsBtn = new QPushButton("Запустить DFS (Обход в глубину)");
        auto* topoBtn = new QPushButton("Топологическая сортировка (M-2.8)");
        auto* clearAnimBtn = new QPushButton("Сбросить подсветку");

        algoLayout->addWidget(startNodeInput_);
        algoLayout->addWidget(bfsBtn);
        algoLayout->addWidget(dfsBtn);
        algoLayout->addWidget(topoBtn);
        algoLayout->addWidget(clearAnimBtn);
        panel->addWidget(algoGroup);

        // 5. Журнал событий
        logOutput_ = new QTextEdit();
        logOutput_->setReadOnly(true);
        panel->addWidget(new QLabel("Журнал операций и результаты:"));
        panel->addWidget(logOutput_);

        mainLayout->addWidget(container, 1);
        mainLayout->addLayout(panel);

        animTimer_ = new QTimer(this);
        connect(animTimer_, &QTimer::timeout, this, &MainWindow::stepAnimation);

        connect(resetBtn, &QPushButton::clicked, this, &MainWindow::initController);
        connect(presetBtn, &QPushButton::clicked, this, &MainWindow::loadDemoPreset);
        connect(meshCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::updateMeshStyle);
        connect(themeCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::updateTheme);

        connect(addVertBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto v = vertexInput_->text().trimmed().toStdString();
                if (!v.empty()) {
                    controller_->addVertex(v);
                    log("Добавлена вершина: " + v);
                    refresh3DScene();
                }
            });
        });

        connect(remVertBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto v = vertexInput_->text().trimmed().toStdString();
                if (!v.empty()) {
                    controller_->removeVertex(v);
                    log("Удалена вершина: " + v);
                    refresh3DScene();
                }
            });
        });

        connect(addEdgeBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto u = fromInput_->text().trimmed().toStdString();
                auto v = toInput_->text().trimmed().toStdString();
                double w = weightInput_->text().toDouble();
                if (!u.empty() && !v.empty()) {
                    controller_->addEdge(u, v, w);
                    log("Добавлено ребро: " + u + (controller_->isDirected() ? " -> " : " <-> ") + v);
                    refresh3DScene();
                }
            });
        });

        connect(remEdgeBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto u = fromInput_->text().trimmed().toStdString();
                auto v = toInput_->text().trimmed().toStdString();
                if (!u.empty() && !v.empty()) {
                    controller_->removeEdge(u, v);
                    log("Удалено ребро: " + u + " - " + v);
                    refresh3DScene();
                }
            });
        });

        connect(bfsBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto start = startNodeInput_->text().trimmed().toStdString();
                auto t0 = std::chrono::high_resolution_clock::now();
                auto order = controller_->bfs(start);
                auto t1 = std::chrono::high_resolution_clock::now();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
                startAnimation("BFS", order, us);
            });
        });

        connect(dfsBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto start = startNodeInput_->text().trimmed().toStdString();
                auto t0 = std::chrono::high_resolution_clock::now();
                auto order = controller_->dfs(start);
                auto t1 = std::chrono::high_resolution_clock::now();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
                startAnimation("DFS", order, us);
            });
        });

        connect(topoBtn, &QPushButton::clicked, this, [this]() {
            execSafe([this]() {
                auto t0 = std::chrono::high_resolution_clock::now();
                auto order = controller_->topoSort();
                auto t1 = std::chrono::high_resolution_clock::now();
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
                startAnimation("Топологическая сортировка", order, us);
            });
        });

        connect(clearAnimBtn, &QPushButton::clicked, this, [this]() {
            animTimer_->stop();
            animSequence_.clear();
            animStep_ = 0;
            refresh3DScene();
        });

        initController();
        loadDemoPreset();
    }

private:
    template <typename Func>
    void execSafe(Func&& f) {
        try {
            f();
        } catch (const std::exception& ex) {
            QMessageBox::warning(this, "Ошибка выполнения", ex.what());
            log(std::string("ОШИБКА: ") + ex.what());
        }
    }

    void log(const std::string& msg) {
        logOutput_->append(QString::fromStdString(msg));
    }

    void initController() {
        animTimer_->stop();
        animSequence_.clear();
        animStep_ = 0;

        bool dir = directedCheck_->isChecked();
        int idx = typeCombo_->currentIndex();
        if (idx == 0) {
            controller_ = std::make_unique<GraphControllerImpl<int>>(dir);
        } else if (idx == 1) {
            controller_ = std::make_unique<GraphControllerImpl<std::string>>(dir);
        } else {
            controller_ = std::make_unique<GraphControllerImpl<double>>(dir);
        }
        log("Создан новый граф. Ориентированный: " + std::string(dir ? "Да" : "Нет"));
        refresh3DScene();
    }

    void loadDemoPreset() {
        initController();
        int idx = typeCombo_->currentIndex();
        if (idx == 1) {
            controller_->addEdge("Core", "Math", 1.0);
            controller_->addEdge("Core", "Utils", 1.0);
            controller_->addEdge("Math", "Physics", 2.0);
            controller_->addEdge("Utils", "Physics", 1.5);
            controller_->addEdge("Utils", "UI", 3.0);
            controller_->addEdge("Physics", "Engine", 4.0);
            controller_->addEdge("UI", "Engine", 2.5);
            startNodeInput_->setText("Core");
        } else {
            controller_->addEdge("5", "2", 1.0);
            controller_->addEdge("5", "0", 1.0);
            controller_->addEdge("4", "0", 1.0);
            controller_->addEdge("4", "1", 1.0);
            controller_->addEdge("2", "3", 1.0);
            controller_->addEdge("3", "1", 1.0);
            startNodeInput_->setText("5");
        }
        log("Загружен демонстрационный DAG-граф.");
        refresh3DScene();
    }

    void updateMeshStyle(int idx) {
        QAbstract3DSeries::Mesh m = QAbstract3DSeries::MeshSphere;
        if (idx == 1) m = QAbstract3DSeries::MeshCube;
        else if (idx == 2) m = QAbstract3DSeries::MeshPyramid;
        else if (idx == 3) m = QAbstract3DSeries::MeshCylinder;
        nodeSeries_->setMesh(m);
        highlightSeries_->setMesh(m);
    }

    void updateTheme(int idx) {
        if (idx == 0) scatter_->activeTheme()->setType(Q3DTheme::ThemeEbony);
        else if (idx == 1) scatter_->activeTheme()->setType(Q3DTheme::ThemeQt);
        else if (idx == 2) scatter_->activeTheme()->setType(Q3DTheme::ThemeRetro);
        else scatter_->activeTheme()->setType(Q3DTheme::ThemePrimaryColors);
    }

    QVector3D computeNodePosition(size_t i, size_t total) const {
        if (total == 1) return QVector3D(0.0f, 0.0f, 0.0f);
        const float radius = 7.5f;
        const float goldenAngle = std::numbers::pi_v<float> * (3.0f - std::sqrt(5.0f));
        float y = 1.0f - (static_cast<float>(i) / static_cast<float>(total - 1)) * 2.0f;
        float r = std::sqrt(std::max(0.0f, 1.0f - y * y));
        float theta = goldenAngle * static_cast<float>(i);
        return QVector3D(std::cos(theta) * r * radius, y * radius * 0.75f, std::sin(theta) * r * radius);
    }

    void refresh3DScene() {
        scatter_->removeCustomItems();
        labels_.clear();
        normalNodeNames_.clear();

        auto verts = controller_->getVertices();
        auto edges = controller_->getEdges();

        auto* nodeData = new QScatterDataArray();
        auto* highData = new QScatterDataArray();
        auto* edgeData = new QScatterDataArray();
        auto* arrowData = new QScatterDataArray();

        avl_tree<GraphNode<std::string>, QVector3D> positions;

        for (size_t i = 0; i < verts.size(); ++i) {
            QVector3D pos = computeNodePosition(i, verts.size());
            positions.add(GraphNode<std::string>{verts[i]}, pos);

            bool isHighlighted = false;
            int stepOrder = -1;
            for (size_t k = 0; k < animStep_ && k < animSequence_.size(); ++k) {
                if (animSequence_[k] == verts[i]) {
                    isHighlighted = true;
                    stepOrder = static_cast<int>(k) + 1;
                    break;
                }
            }

            if (isHighlighted) {
                *highData << QScatterDataItem(pos);
            } else {
                *nodeData << QScatterDataItem(pos);
                normalNodeNames_.push_back(verts[i]);
            }

            // Создаем четкую неперекрываемую табличку прямо над вершиной
            auto* label = new QCustom3DLabel();
            std::string labelText = "  " + verts[i] + "  ";
            if (isHighlighted) {
                labelText = "  " + verts[i] + " [#" + std::to_string(stepOrder) + "]  ";
            }
            label->setText(QString::fromStdString(labelText));
            label->setFont(QFont("Arial", 22, QFont::Bold));

            // Смещаем строго вверх и чуть наружу от центра, чтобы сфера никогда не перекрывала текст
            QVector3D dirOut = pos.normalized();
            label->setPosition(pos + QVector3D(0.0f, 1.35f, 0.0f) + dirOut * 0.4f);
            label->setScaling(QVector3D(0.35f, 0.35f, 0.35f));
            label->setFacingCamera(true);
            label->setBackgroundEnabled(true);
            label->setBorderEnabled(true);

            if (isHighlighted) {
                label->setBackgroundColor(QColor(215, 140, 0));
                label->setTextColor(Qt::black);
            } else {
                label->setBackgroundColor(QColor(20, 30, 48));
                label->setTextColor(QColor(0, 230, 255));
            }

            scatter_->addCustomItem(label);
            labels_.push_back(label);
        }

        // Рисуем гладкие линии ребер
        for (const auto& [u, v, w] : edges) {
            QVector3D p1 = positions.get(GraphNode<std::string>{u});
            QVector3D p2 = positions.get(GraphNode<std::string>{v});

            constexpr int segments = 45;
            for (int s = 3; s < segments - 2; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(segments);
                QVector3D pt = p1 * (1.0f - t) + p2 * t;
                // Последние 20% перед входом в вершину V подсвечиваем оранжево-красным как наконечник стрелки
                if (controller_->isDirected() && t >= 0.78f) {
                    *arrowData << QScatterDataItem(pt);
                } else {
                    *edgeData << QScatterDataItem(pt);
                }
            }
        }

        nodeSeries_->dataProxy()->resetArray(nodeData);
        highlightSeries_->dataProxy()->resetArray(highData);
        edgeSeries_->dataProxy()->resetArray(edgeData);
        arrowSeries_->dataProxy()->resetArray(arrowData);
    }

    void startAnimation(const std::string& name, const std::vector<std::string>& seq, long long microseconds) {
        animTimer_->stop();
        animSequence_ = seq;
        animStep_ = 0;

        std::ostringstream oss;
        oss << name << " (" << microseconds << " мкс): ";
        for (size_t i = 0; i < seq.size(); ++i) {
            oss << seq[i] << (i + 1 < seq.size() ? " -> " : "");
        }
        log(oss.str());

        if (!seq.empty()) {
            animTimer_->start(450);
        }
    }

    void stepAnimation() {
        if (animStep_ < animSequence_.size()) {
            ++animStep_;
            refresh3DScene();
        } else {
            animTimer_->stop();
        }
    }

    void onNodeClicked(int index) {
        if (index < 0 || static_cast<size_t>(index) >= normalNodeNames_.size()) return;
        std::string clicked = normalNodeNames_[index];
        nodeSeries_->setItemLabelFormat(QString::fromStdString("Вершина: " + clicked));
        vertexInput_->setText(QString::fromStdString(clicked));
        startNodeInput_->setText(QString::fromStdString(clicked));
        log("Выбрана вершина в 3D-сцене: " + clicked);
    }
};

#include "ui.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}

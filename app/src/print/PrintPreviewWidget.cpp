// PrintPreviewWidget.cpp
#include "PrintPreviewWidget.h"
#include "ImageGenerator.h"
#include "ImageProcessor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QPainter>

PrintPreviewWidget::PrintPreviewWidget(QWidget* parent)
    : QWidget(parent)
    , m_zoomFactor(1.0)
    // , m_printColorMode(ColorMode::BlackTextOnWhiteBackground)
    // , m_isTextMode(false)
{
    auto* mainLayout = new QVBoxLayout(this);

    // === Панель инструментов ===
    auto* toolbar = new QHBoxLayout();

    // Кнопки масштабирования
    auto* zoomInBtn = new QPushButton("➕ Увеличить", this);
    auto* zoomOutBtn = new QPushButton("➖ Уменьшить", this);
    auto* zoomFitBtn = new QPushButton("🔍 По размеру", this);
    auto* zoomActualBtn = new QPushButton("1:1 Реальный размер", this);

    // Кнопка печати
    auto* printButton = new QPushButton("🖨️ Печать", this);
    printButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");

    auto* closeButton = new QPushButton("✖ Закрыть", this);

    connect(zoomInBtn, &QPushButton::clicked, this, &PrintPreviewWidget::zoomIn);
    connect(zoomOutBtn, &QPushButton::clicked, this, &PrintPreviewWidget::zoomOut);
    connect(zoomFitBtn, &QPushButton::clicked, this, &PrintPreviewWidget::zoomToFit);
    connect(zoomActualBtn, &QPushButton::clicked, this, &PrintPreviewWidget::zoomToActualSize);
    connect(printButton, &QPushButton::clicked, this, &PrintPreviewWidget::onPrintButtonClicked);
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    toolbar->addWidget(zoomInBtn);
    toolbar->addWidget(zoomOutBtn);
    toolbar->addWidget(zoomFitBtn);
    toolbar->addWidget(zoomActualBtn);
    toolbar->addStretch();
    toolbar->addWidget(printButton);
    toolbar->addWidget(closeButton);

    // Слайдер масштаба
    auto* zoomLayout = new QHBoxLayout();
    zoomLayout->addWidget(new QLabel("Масштаб:", this));

    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setRange(10, 400);  // 10% - 400%
    m_zoomSlider->setValue(100);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &PrintPreviewWidget::onZoomSliderChanged);

    m_zoomLabel = new QLabel("100%", this);
    m_zoomLabel->setMinimumWidth(50);

    zoomLayout->addWidget(m_zoomSlider);
    zoomLayout->addWidget(m_zoomLabel);

    // === Область просмотра ===
    m_scrollArea = new QScrollArea(this);
    m_imageLabel = new QLabel();
    m_imageLabel->setScaledContents(false);
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(false);  // Отключаем, управляем вручную

    // Собираем всё вместе
    mainLayout->addLayout(toolbar);
    mainLayout->addLayout(zoomLayout);
    mainLayout->addWidget(m_scrollArea);

    setWindowTitle("Предпросмотр печати");
    resize(500, 600);
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint);
}

QPixmap PrintPreviewWidget::convertForDisplay(const QImage& image) {
    qDebug() << "convertForDisplay: конвертация изображения"
             << image.width() << "x" << image.height();

    // Конвертируем в RGB32 для корректного отображения
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB32);
    return QPixmap::fromImage(rgbImage);
}

void PrintPreviewWidget::showPreview(const QImage& image) {
    if (image.isNull()) {
        qDebug() << "Ошибка: пустое изображение";
        m_imageLabel->clear();
        return;
    }

    qDebug() << "showPreview: изображение" << image.width() << "x" << image.height();

    // === ОЧИСТКА ПРЕДЫДУЩЕГО СОСТОЯНИЯ ===
    // 1. Очищаем label
    m_imageLabel->clear();

    // 2. Сбрасываем pixmap
    m_originalPixmap = QPixmap();

    // 3. Принудительно обновляем scrollArea
    m_scrollArea->takeWidget();
    m_scrollArea->setWidget(m_imageLabel);

    // Сохраняем оригинальный pixmap
    m_originalPixmap = convertForDisplay(image);

    // Устанавливаем масштаб по умолчанию (подогнать под окно)
    // zoomToFit();
    zoomToActualSize();

    setWindowTitle(QString("Предпросмотр печати (%1x%2 точек) - нажмите 'Печать' для отправки")
                       .arg(image.width())
                       .arg(image.height()));

    show();
    raise();
}

void PrintPreviewWidget::showPreview(const QString& text,
                                     const PrintSettings& settings,
                                     ColorMode mode) {
    // Сохраняем данные для печати (текстовый режим)
    // m_printText = text;
    // m_printSettings = settings;
    // m_printColorMode = mode;
    // m_isTextMode = true;
    // m_hasPrintData = true;

    ImageGenerator generator(settings);
    QImage image = generator.generateFromText(text);
    QImage processed = ImageProcessor::prepareForPrint(image, mode);

    // Вызываем основной метод showPreview с изображением
    // Но переопределяем поведение, чтобы сохранить текстовые данные
    if (processed.isNull()) {
        qDebug() << "Ошибка: пустое изображение";
        m_imageLabel->clear();
        return;
    }

    qDebug() << "showPreview (текст):" << text.left(50) << "... размер:"
             << processed.width() << "x" << processed.height();

    // Сохраняем изображение для отображения
    // m_printImage = processed;
    m_originalPixmap = convertForDisplay(processed);

    zoomToActualSize();

    setWindowTitle(QString("Предпросмотр печати (%1x%2 точек) - нажмите 'Печать' для отправки")
                       .arg(processed.width())
                       .arg(processed.height()));

    show();
    raise();
}

// void PrintPreviewWidget::setPrintData(const QImage& image,
//                                       ColorMode mode,
//                                       const PrintSettings& settings) {
//     m_printImage = image;
//     m_printColorMode = mode;
//     m_printSettings = settings;
//     m_isTextMode = false;
//     m_hasPrintData = true;
// }

void PrintPreviewWidget::onPrintButtonClicked() {
    qDebug() << "Нажата кнопка 'Печать'";

    // if (!m_hasPrintData) {
    //     qDebug() << "Нет данных для печати";
    //     return;
    // }

    emit printRequested();

    // if (m_isTextMode) {
    //     // Отправляем сигнал для текстовой печати
    //     emit printTextRequested(m_printText, m_printColorMode, m_printSettings);
    //     qDebug() << "Сигнал printTextRequested отправлен";
    // } else {
    //     // Отправляем сигнал для печати изображения
    //     emit printRequested(m_printImage, m_printColorMode, m_printSettings);
    //     qDebug() << "Сигнал printRequested отправлен";
    // }

    // Закрываем окно предпросмотра
    close();
}

void PrintPreviewWidget::zoomIn() {
    setZoomFactor(m_zoomFactor * 1.25);
}

void PrintPreviewWidget::zoomOut() {
    setZoomFactor(m_zoomFactor * 0.8);
}

void PrintPreviewWidget::zoomToFit() {
    if (m_originalPixmap.isNull()) return;

    // Вычисляем масштаб, чтобы изображение поместилось в scrollArea
    int availableWidth = m_scrollArea->viewport()->width() - 20;  // Небольшой запас
    int availableHeight = m_scrollArea->viewport()->height() - 20;

    if (availableWidth <= 0 || availableHeight <= 0) {
        // Если scrollArea ещё не отрисован, используем размер окна
        availableWidth = width() - 100;
        availableHeight = height() - 150;
    }

    double scaleX = static_cast<double>(availableWidth) / m_originalPixmap.width();
    double scaleY = static_cast<double>(availableHeight) / m_originalPixmap.height();
    double fitZoom = qMin(scaleX, scaleY);

    // Ограничиваем масштаб разумными пределами
    fitZoom = qBound(0.1, fitZoom, 10.0);

    setZoomFactor(fitZoom);
}

void PrintPreviewWidget::zoomToActualSize() {
    setZoomFactor(1.0);  // 1:1 - реальный размер
}

void PrintPreviewWidget::setZoomFactor(double factor) {
    // Ограничиваем масштаб (10% - 400%)
    m_zoomFactor = qBound(0.1, factor, 4.0);

    // Обновляем слайдер (без вызова сигнала)
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(static_cast<int>(m_zoomFactor * 100));
    m_zoomSlider->blockSignals(false);

    // Обновляем отображение
    updateScaledPixmap();
    updateZoomLabel();
}

void PrintPreviewWidget::onZoomSliderChanged(int value) {
    setZoomFactor(value / 100.0);
}

void PrintPreviewWidget::updateScaledPixmap() {
    if (m_originalPixmap.isNull()) return;

    // Масштабируем pixmap
    int newWidth = static_cast<int>(m_originalPixmap.width() * m_zoomFactor);
    int newHeight = static_cast<int>(m_originalPixmap.height() * m_zoomFactor);

    // Используем SmoothTransformation для плавного масштабирования
    QPixmap scaled = m_originalPixmap.scaled(newWidth, newHeight,
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);

    m_imageLabel->setPixmap(scaled);
    m_imageLabel->setFixedSize(scaled.size());

    qDebug() << "Масштаб:" << QString::number(m_zoomFactor * 100, 'f', 0) + "%"
             << "Размер:" << scaled.width() << "x" << scaled.height();
}

void PrintPreviewWidget::updateZoomLabel() {
    m_zoomLabel->setText(QString::number(static_cast<int>(m_zoomFactor * 100)) + "%");
}

void PrintPreviewWidget::clear() {
    m_imageLabel->clear();
    m_originalPixmap = QPixmap();
}

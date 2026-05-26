// PrintPreviewWidget.h
#pragma once
#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QImage>
#include <QPixmap>
#include <QSlider>
#include <QToolBar>

#include "ImageGenerator.h"
#include "ImageProcessor.h"

class PrintPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PrintPreviewWidget(QWidget* parent = nullptr);

    void showPreview(const QImage& image);
    void showPreview(const QString& text,
                     const PrintSettings& settings,
                     ColorMode mode);

    void clear();

    // Управление масштабом
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomToActualSize();
    void setZoomFactor(double factor);

    // Установка данных для печати (для callback)
    // void setPrintData(const QImage& image,
    //                   ColorMode mode = ColorMode::BlackTextOnWhiteBackground,
    //                   const PrintSettings& settings = PrintSettings());

signals:
    // Сигнал для запроса печати
    void printRequested();

    // Упрощённый сигнал для текстовой печати
    void printTextRequested();

private slots:
    void onZoomSliderChanged(int value);
    void onPrintButtonClicked();   // Обработчик кнопки "Печать"

private:
    QScrollArea* m_scrollArea;
    QLabel* m_imageLabel;
    QSlider* m_zoomSlider;
    QLabel* m_zoomLabel;

    QPixmap m_originalPixmap;   // Храним оригинальный (немасштабированный) pixmap
    double m_zoomFactor;         // Текущий коэффициент масштаба

    // Храним данные для печати
    // QImage m_printImage;
    // ColorMode m_printColorMode;
    // PrintSettings m_printSettings;
    // bool m_hasPrintData = false;

    // Для текстовой печати
    // QString m_printText;
    // bool m_isTextMode = false;

    // Конвертирует монохромное изображение в формат, пригодный для отображения
    QPixmap convertForDisplay(const QImage& image);

    void updateScaledPixmap();   // Обновляет отображение с текущим масштабом
    void updateZoomLabel();       // Обновляет текст с zoom
};

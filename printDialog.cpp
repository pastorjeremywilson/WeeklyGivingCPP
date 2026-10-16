//
// Created by jeremy on 8/18/26.
//

#include "printDialog.h"
#include "gui.h"
#include "autoSelectWidgets.h"
#include <iostream>
#include <algorithm>
#include <QDialog>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPrinterInfo>
#include <QHash>
#include <QVariant>
#include <QMarginsF>
#include <QMetaType>
#include <QPushButton>
#include <QPainter>
#include <QFile>
#ifdef Q_OS_WIN
    #include <windows.h>
    #include <winspool.h>
#endif

#pragma comment(lib, "winspool.lib")

CustomPrintDialog::CustomPrintDialog(
        QWidget* parent,
        MainWindow* mainWindow,
        bool landscape,
        float pageMarginsHorizontal,
        float pageMarginsVertical)
    : QDialog(parent),
      mMainWindow(mainWindow),
      mLandscape(landscape),
      mPageMarginsHorizontal(pageMarginsHorizontal),
      mPageMarginsVertical(pageMarginsVertical),
      mCurrentPage(0)
{
    /**
     * Creates a print dialog with a preview image based on the page orientation and margins supplied
     * @param mainWindow The current instance of MainWindow
     * @param landscape Whether the document is landscape orientation
     * @param pageMarginsHorizontal Horizontal page margins
     * @param pageMarginsVertical Vertical page margins
     */
    mPrinterProperties["name"] = QString("");
    mPrinterProperties["letterPageSizePx"] = QRectF();
    mPrinterProperties["printableRectInch"] = QRectF();
    mPrinterProperties["printableRectPx"] = QRectF();
    mPrinterProperties["printableMargin"] = QVariant::fromValue(QMarginsF(0.0, 0.0, 0.0, 0.0));
    mPrinterProperties["centeredPrintableRectPx"] = QRectF();

    setWindowTitle("Print Bulletin");
    setObjectName("container");
    setAttribute(Qt::WA_StyledBackground, true);

    initComponents();
    setPrinterProperties();
}

void CustomPrintDialog::initComponents() {
    /**
     * Initiates and lays out the GUI components of the print dialog
     */
    auto* layout = new QHBoxLayout(this);

    auto* previewWidget = new QWidget(this);
    layout->addWidget(previewWidget);
    previewWidget->setObjectName("container");
    auto* previewLayout = new QVBoxLayout(previewWidget);

    auto previewLabel = new QLabel("Preview:");
    previewLabel->setFont(mMainWindow->mBoldFont);
    previewLayout->addWidget(previewLabel);

    mPreviewLabel = new QLabel();
    previewLayout->addWidget(mPreviewLabel);

    std::cout << "making divider" << std::endl;
    QWidget divider;
    divider.setObjectName("divider");
    divider.setStyleSheet("border-left: 2px solid #105010;");
    divider.setFixedWidth(2);
    divider.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    layout->addWidget(&divider);

    auto* printOptionsWidget = new QWidget();
    layout->addWidget(printOptionsWidget);
    printOptionsWidget->setObjectName("container");
    auto* printOptionsLayout = new QVBoxLayout(printOptionsWidget);
    printOptionsLayout->setSpacing(0);

    auto* printButton = new QToolButton();
    printOptionsLayout->addWidget(printButton);
    printOptionsLayout->addSpacing(20);
    printButton->setText("Print");
    printButton->setIcon(QIcon(":/resources/print.svg"));
    printButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
    printButton->setFont(mMainWindow->mBoldFont);
    printButton->setIconSize(QSize(64, 64));
    printButton->setFixedWidth(72);

    auto* printerWidget = new QWidget();
    printOptionsLayout->addWidget(printerWidget);
    printOptionsLayout->addSpacing(20);
    auto* printerLayout = new QHBoxLayout(printerWidget);
    printerLayout->setContentsMargins(0, 0, 0, 0);

    auto* printerLabel = new QLabel("Printer");
    printerLayout->addWidget(printerLabel);
    printerLabel->setFont(mMainWindow->mBoldFont);

    mPrinterCombobox = new QComboBox();
    printerLayout->addWidget(mPrinterCombobox);
    mPrinterCombobox->setMinimumHeight(32);
    mPrinterCombobox->setFont(mMainWindow->mStandardFont);
    QList<QPrinterInfo> printers = QPrinterInfo::availablePrinters();

    if (!printers.isEmpty()) {
        QString defaultPrinter = printers.at(0).printerName();
        for (QPrinterInfo& printer : printers) {
            mPrinterCombobox->addItem(printer.printerName());
            if (printer.isDefault()) {
                defaultPrinter = printer.printerName();
            }
        }
        mPrinterCombobox->setCurrentText(defaultPrinter);
    }

    auto* copiesWidget = new QWidget();
    printOptionsLayout->addWidget(copiesWidget);
    printOptionsLayout->addSpacing(20);
    auto* copiesLayout = new QHBoxLayout(copiesWidget);
    copiesLayout->setContentsMargins(0, 0, 0, 0);

    auto* copiesLabel = new QLabel("Copies");
    copiesLayout->addWidget(copiesLabel);
    copiesLayout->addStretch();
    copiesLabel->setFont(mMainWindow->mBoldFont);

    mCopiesSpinbox = new AutoSelectSpinBox();
    copiesLayout->addWidget(mCopiesSpinbox);
    mCopiesSpinbox->setMinimumSize(100, 40);
    mCopiesSpinbox->setRange(1, 500);
    mCopiesSpinbox->setFont(mMainWindow->mStandardFont);

    auto* orientationWidget = new QWidget();
    printOptionsLayout->addWidget(orientationWidget);
    printOptionsLayout->addSpacing(20);
    auto* orientationLayout = new QHBoxLayout(orientationWidget);
    orientationLayout->setContentsMargins(0, 0, 0, 0);

    auto* orientationLabel = new QLabel("Orientation");
    orientationLayout->addWidget(orientationLabel);
    orientationLayout->addStretch();
    orientationLabel->setFont(mMainWindow->mBoldFont);

    auto* portraitRadioButton = new QRadioButton("Portrait");
    orientationLayout->addWidget(portraitRadioButton);
    orientationLayout->addSpacing(20);
    portraitRadioButton->setFont(mMainWindow->mStandardFont);

    auto* landscapeRadioButton = new QRadioButton("Landscape");
    orientationLayout->addWidget(landscapeRadioButton);
    landscapeRadioButton->setFont(mMainWindow->mStandardFont);

    mOrientationButtonGroup = new QButtonGroup();
    mOrientationButtonGroup->setExclusive(true);
    mOrientationButtonGroup->addButton(portraitRadioButton, 0);
    mOrientationButtonGroup->addButton(landscapeRadioButton, 1);

    if (mLandscape) {
        landscapeRadioButton->setChecked(true);
    } else {
        portraitRadioButton->setChecked(true);
    }

    auto* duplexWidget = new QWidget();
    printOptionsLayout->addWidget(duplexWidget);
    printOptionsLayout->addSpacing(20);
    auto* duplexLayout = new QHBoxLayout(duplexWidget);
    duplexLayout->setContentsMargins(0, 0, 0, 0);

    auto* duplexLabel = new QLabel("2-Sided Printing");
    duplexLayout->addWidget(duplexLabel);
    duplexLayout->addStretch();
    duplexLabel->setFont(mMainWindow->mBoldFont);

    mDuplexCombobox = new QComboBox();
    duplexLayout->addWidget(mDuplexCombobox);
    mDuplexCombobox->setObjectName("duplexCombobox");
    mDuplexCombobox->setFont(mMainWindow->mStandardFont);
    mDuplexCombobox->setMinimumHeight(32);
    mDuplexCombobox->addItem(QIcon(":/resources/one_sided.svg"), "Print 1-Sided");
    mDuplexCombobox->addItem(QIcon(":/resources/flip_long_edge.svg"), "Flip on Long Edge");
    mDuplexCombobox->addItem(QIcon(":/resources/flip_short_edge.svg"), "Flip on Short Edge");
    mDuplexCombobox->setIconSize(QSize(36, 36));

    auto* rangeWidget = new QWidget();
    printOptionsLayout->addWidget(rangeWidget);
    auto* rangeLayout = new QHBoxLayout(rangeWidget);
    rangeLayout->setContentsMargins(0, 0, 0, 0);

    auto* rangeLabel = new QLabel("Pages");
    rangeLayout->addWidget(rangeLabel);
    rangeLayout->addStretch();
    rangeLabel->setFont(mMainWindow->mBoldFont);

    mRangeLineEdit = new AutoSelectLineEdit();
    rangeLayout->addWidget(mRangeLineEdit);
    mRangeLineEdit->setMinimumHeight(32);
    mRangeLineEdit->setFont(mMainWindow->mStandardFont);

    auto* detailsLabel = new QLabel("(i.e. '2', '1-3', '1, 3', or leave blank to print all pages)");
    printOptionsLayout->addWidget(detailsLabel);
    printOptionsLayout->addStretch();
    detailsLabel->setFont(mMainWindow->mStandardFont);
    detailsLabel->setAlignment(Qt::AlignRight);

    connect(printButton, &QPushButton::clicked, this, &CustomPrintDialog::doPrint);
    connect(mPrinterCombobox, &QComboBox::currentIndexChanged, this, &CustomPrintDialog::setPrinterProperties);
    connect(mOrientationButtonGroup, &QButtonGroup::buttonClicked, this, &CustomPrintDialog::orientationChanged);
}

void CustomPrintDialog::setPrinterProperties() {
    /**
     * Gets the physical DPI of the current printer and populates mPrinterProperties with its name and page size
     * and calculates the printable area of the page based on the printer's own margins
     * @return void
     */
    const QString printerName = mPrinterCombobox->currentText();

    mMainWindow->printer->setPrinterName(printerName);

    QRectF pageRectInch = mMainWindow->printer->pageRect(QPrinter::Unit::Inch);

    int dpi_x = mMainWindow->printer->physicalDpiX();
    int dpi_y = mMainWindow->printer->physicalDpiY();

    double unprintableMarginX = (8.5 - pageRectInch.width()) / 2;
    double unprintableMarginY = (11 - pageRectInch.width()) / 2;

    mPrinterProperties["name"] = printerName;
    mPrinterProperties["letterPageSizePx"] = QRectF(
        0,
        0,
        8.5 * dpi_x,
        11 * dpi_y
    );
    mPrinterProperties["printableRectInch"] = QRectF(
        unprintableMarginX,
        unprintableMarginY,
        pageRectInch.width(),
        pageRectInch.height()
    );
    mPrinterProperties["printableRectPx"] = QRectF(
        unprintableMarginX * dpi_x,
        unprintableMarginY * dpi_y,
        pageRectInch.width() * dpi_x,
        pageRectInch.height() * dpi_y
    );
    QMarginsF margins(
    mPrinterProperties["printableRectPx"].value<QRectF>().x()
        - mPrinterProperties["letterPageSizePx"].value<QRectF>().x(),
    mPrinterProperties["printableRectPx"].value<QRectF>().y()
        - mPrinterProperties["letterPageSizePx"].value<QRectF>().y(),
    mPrinterProperties["letterPageSizePx"].value<QRectF>().width()
        - mPrinterProperties["printableRectPx"].value<QRectF>().x()
            + mPrinterProperties["printableRectPx"].value<QRectF>().width(),
    mPrinterProperties["letterPageSizePx"].value<QRectF>().height()
        - mPrinterProperties["printableRectPx"].value<QRectF>().y()
            + mPrinterProperties["printableRectPx"].value<QRectF>().height()
    );
    mPrinterProperties["printableMargins"] = QVariant(QMetaType::fromType<QMarginsF>(), &margins);
    double maxXMargin = (std::max)(
        mPrinterProperties["printableMargins"].value<QRectF>().left(),
        mPrinterProperties["printableMargins"].value<QRectF>().right()
    );
    double maxYMargin = (std::max)(
        mPrinterProperties["printableMargins"].value<QRectF>().top(),
        mPrinterProperties["printableMargins"].value<QRectF>().bottom());
    mPrinterProperties["centeredPrintableRectPx"] = QRectF(
        maxXMargin,
        maxYMargin,
        mPrinterProperties["letterPageSizePx"].value<QRectF>().width() - (maxXMargin * 2),
        mPrinterProperties["letterPageSizePx"].value<QRectF>().height() - (maxYMargin * 2)
    );
    makePreview();
}

void CustomPrintDialog::drawPreview(QImage image) {
    /**
     * Creates a sized pixmap based on a QImage representation of the page to be previewed, adding border and
     * shadow effects to the page.
     * @param image the QImage to set as a preview
     * @return void
     */
    mPreviewLabel->clear();
    int paperWidth;
    int paperHeight;
    if (mLandscape) {
        paperWidth = 600 / 8.5 * 11.0;
        paperHeight = 600;
    } else {
        paperWidth = 600;
        paperHeight = 600 / 8.5 * 11.0;
    }

    QPixmap pixmap(paperWidth + 4, paperHeight + 4);
    auto* painter = new QPainter(&pixmap);
    auto pen = QPen();
    pen.setWidth(0);
    painter->setPen(pen);
    painter->fillRect(0, 0, paperWidth + 4, paperHeight + 4, Qt::GlobalColor::white);
    painter->fillRect(4, 4, paperWidth, paperHeight, Qt::GlobalColor::darkGray);
    painter->fillRect(0, 0, paperWidth, paperHeight, Qt::GlobalColor::white);
    painter->drawImage(0, 0, image);
    pen.setWidth(2);
    painter->setPen(pen);
    painter->setBrush(Qt::GlobalColor::transparent);
    painter->drawRect(0, 0, paperWidth, paperHeight);
    painter->end();

    mPreviewLabel->setPixmap(pixmap);
}

void CustomPrintDialog::orientationChanged() {
    /**
     * Changes the value of mLandscape and redraws the preview
     * @return void
     */
    if (mOrientationButtonGroup->checkedId() == 0) {
        mLandscape = false;
    } else {
        mLandscape = true;
    }
    drawPreview(mPages.at(mCurrentPage));
}

std::vector<int> CustomPrintDialog::parsePageRange(QString pageRange) {
    /**
     * Parses the user-inputted pages to print into a vector of integers representing the pages to be printed
     * @param pageRange the user's inputted print range
     * @return vector of integers for the pages to be printed, or a single -1 if parsing failed
     */
    std::vector<int> pagesToPrint;
    if (pageRange.indexOf(",") != -1) {
        // First, split the pageRange string by commas and iterate through the result
        QList<QString> pageRangeSplit = pageRange.split(",");
        for (QString& page : pageRangeSplit) {
            if (page.indexOf("-") != -1) {
                // There a dash in this segment so determine the pages in this range
                QList<QString> thisSplit = page.split("-");
                if (thisSplit.size() != 2) return std::vector{-1};

                bool ok = false;
                int low = thisSplit.at(0).trimmed().toInt(&ok) - 1;
                if (!ok) return {-1};
                int high = thisSplit.at(1).trimmed().toInt(&ok) - 1;
                if (!ok) return {-1};

                if (low > high) {
                    low = thisSplit.at(1).toInt();
                    high = thisSplit.at(0).toInt();
                }

                for (int i = low; i <= high; i++) {
                    pagesToPrint.emplace_back(i);
                }
            } else {
                bool ok = false;
                int thisPage = page.trimmed().toInt(&ok);
                if (!ok) return {-1};
            }
        }
    } else if (pageRange.indexOf("-") != -1) {
        // there were no commas but there is a dash - figure out the page range
        QList<QString> thisSplit = pageRange.trimmed().split("-");
        if (thisSplit.size() != 2) return std::vector{-1};
        bool ok = false;
        int low = thisSplit.at(0).trimmed().toInt(&ok) - 1;
        if (!ok) return {-1};
        int high = thisSplit.at(1).trimmed().toInt(&ok) - 1;
        if (!ok) return {-1};

        for (int i = low; i <= high; i++) {
            pagesToPrint.emplace_back(i);
        }
    } else {
        // No dashes or commas. This is a simple page to print.
        bool ok = false;
        pagesToPrint.emplace_back(pageRange.trimmed().toInt(&ok) - 1);
        if (!ok) return {-1};
    }

    return pagesToPrint;
}

void CustomPrintDialog::makePreview() {
    /**
     * Creates a pixmap and sends its painter to MainWindow's createReport function. Draws the pixmap, adding border and
     * shadow effects to the page.
     * @return void
     */
    if (mOrientationButtonGroup->checkedId() == 0) {
        mLandscape = false;
        mMainWindow->printer->setPageOrientation(QPageLayout::Orientation::Portrait);
    } else {
        mLandscape = true;
        mMainWindow->printer->setPageOrientation(QPageLayout::Orientation::Landscape);
    }

    // Create a pixmap on whose painter createReport will draw
    QPixmap pixmap(
        mMainWindow->printer->pageRect(QPrinter::DevicePixel).width(),
        mMainWindow->printer->pageRect(QPrinter::DevicePixel).height()
    );
    pixmap.fill(Qt::GlobalColor::white);
    QPainter painter(&pixmap);
    mMainWindow->createReport(painter);
    pixmap = pixmap.scaledToWidth(600, Qt::SmoothTransformation);

    // Create a pixmap that is 5 pixels larger than the original pixmap to use for the preview
    QPixmap previewPixmap(pixmap.width() + 5, pixmap.height() + 5);
    previewPixmap.fill(Qt::GlobalColor::transparent);

    // Draw a border for the page
    QPainter previewPainter(&previewPixmap);
    QPen pen(Qt::GlobalColor::black, 2, Qt::SolidLine);
    previewPainter.setPen(pen);

    // Draw a shadow for the page, offset by 5 pixels
    previewPainter.fillRect(
        QRect(5, 5, pixmap.width(), pixmap.height()),
        Qt::GlobalColor::gray
    );

    // Draw the original pixmap
    previewPainter.drawPixmap(pixmap.rect(), pixmap, pixmap.rect());
    previewPainter.drawRect(pixmap.rect());
    previewPainter.end();

    mPreviewLabel->setPixmap(previewPixmap);
}

void CustomPrintDialog::doPrint() {
    /**
     * Sets the name, orientation, and duplex for MainWindow's printer. Sends the painter of the printer
     * to MainWindow's createReport function for printing.
     * @return void
     */
    mMainWindow->printer->setPrinterName(mPrinterProperties["name"].value<QString>());
    mMainWindow->printer->setDocName("Bulletin Builder");

    if (mOrientationButtonGroup->checkedId() == 1) {
        mMainWindow->printer->setPageOrientation(QPageLayout::Orientation::Landscape);
    } else {
        mMainWindow->printer->setPageOrientation(QPageLayout::Orientation::Portrait);
    }

    if (mDuplexCombobox->currentIndex() == 0) {
        mMainWindow->printer->setDuplex(QPrinter::DuplexMode::DuplexNone);
    } else if (mDuplexCombobox->currentIndex() == 1) {
        mMainWindow->printer->setDuplex(QPrinter::DuplexMode::DuplexLongSide);
    } else if (mDuplexCombobox->currentIndex() == 2) {
        mMainWindow->printer->setDuplex(QPrinter::DuplexMode::DuplexShortSide);
    }

    QPainter painter(mMainWindow->printer);
    mMainWindow->createReport(painter);
    this->done(Accepted);
}
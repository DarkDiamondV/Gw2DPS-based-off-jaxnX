#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_configurator.h"
#include <QUrl>
#include <QDesktopServices>
#include <QtDebug>
#include "screenrecorder.h"
#include "settings.h"
#include "global.h"
#include <QSizeGrip>
#include <QToolBar>
#include <QToolButton>
#include "ui_mydialog.h"
#include <QtNetwork>
#include <QUrl>
#include <qcustomplot.h>
#include <qt_windows.h>

using namespace GW2;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_Configurator(this),
    m_MyDialog(this),
    update_Timer(this)
{
    CheckForOldVerison();
    ui->setupUi(this);
    // generate ui and so on
    StartupPref();
    _kc << Qt::Key_Up << Qt::Key_Up << Qt::Key_Down << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right << Qt::Key_Left << Qt::Key_Right << Qt::Key_A << Qt::Key_B;
    _pos = 0;

    QObject::connect(&update_Timer, SIGNAL(timeout()), this, SLOT(UpdateTimer()));

    ScreenRecorder* screenRecorder = new ScreenRecorder;
    DmgMeter* dmgMeter = &screenRecorder->GetDmgMeter();
    screenRecorder->moveToThread(&m_ScreenRecorderThread);

    Ui::Configurator* uiConfig = m_Configurator.ui;

    dmgMeter->moveToThread(&m_ScreenRecorderThread);

    QObject::connect(&m_ScreenRecorderThread, SIGNAL(finished()), screenRecorder, SLOT(deleteLater()));
    QObject::connect(&m_ScreenRecorderThread, SIGNAL(started()), screenRecorder, SLOT(StartRecording()));
    QObject::connect(screenRecorder, SIGNAL(RequestInfoUpdate(QString)), ui->labelInfo, SLOT(setText(QString)));
    QObject::connect(dmgMeter, SIGNAL(RequestTimeUpdate(int)), this, SLOT(UpdateTime(int)));
    QObject::connect(ui->actionReset, SIGNAL(triggered()), dmgMeter, SLOT(Reset()));
    QObject::connect(ui->actionReset, SIGNAL(triggered()), this, SLOT(resetGraph()));
    QObject::connect(ui->actionAutoReset, SIGNAL(triggered(bool)), dmgMeter, SLOT(SetIsAutoResetting(bool)));
    QObject::connect(ui->actionEnableTransparency, SIGNAL(triggered(bool)), this, SLOT(EnableTransparency(bool)));
    QObject::connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(LinkToWebsite()));
    QObject::connect(ui->actionConfig, SIGNAL(triggered()), &m_Configurator, SLOT(exec()));
    // connecting configurator - SPECS display settings
    QObject::connect(uiConfig->checkBoxToolbar, SIGNAL(clicked(bool)), this, SLOT(ShowToolbarChanged()));
    QObject::connect(uiConfig->checkBoxDetails, SIGNAL(clicked(bool)), this, SLOT(ShowDetailsChanged()));
    QObject::connect(uiConfig->checkBoxExtraDetails, SIGNAL(clicked(bool)), this, SLOT(ShowExtraDetailsChanged()));
    QObject::connect(ui->pushButton, SIGNAL(toggled(bool)), this, SLOT(ShowExtraDetailsChanged()));
    QObject::connect(uiConfig->checkBoxOpacity, SIGNAL(clicked(bool)), this, SLOT(ShowOpacityChanged()));
    // connecting configurator - solo display settings
    QObject::connect(uiConfig->checkBoxProfColors, SIGNAL(clicked(bool)), this, SLOT(ProfSettingsChanged()));
    QObject::connect(uiConfig->checkBoxName, SIGNAL(clicked(bool)), this, SLOT(NameChanged()));
    QObject::connect(uiConfig->checkBoxDamage, SIGNAL(clicked(bool)), this, SLOT(DamageChanged()));
    QObject::connect(uiConfig->checkBoxDPS, SIGNAL(clicked(bool)), this, SLOT(DPSChanged()));
    QObject::connect(uiConfig->checkBoxCDamage, SIGNAL(clicked(bool)), this, SLOT(CDamageChanged()));
    QObject::connect(uiConfig->checkBoxCPerDmg, SIGNAL(clicked(bool)), this, SLOT(CPerDmgChanged()));
    QObject::connect(uiConfig->checkBoxCDPS, SIGNAL(clicked(bool)), this, SLOT(CDPSChanged()));
    QObject::connect(uiConfig->checkBox5sDPS, SIGNAL(clicked(bool)), this, SLOT(FiveSecRealDPSChanged()));
    QObject::connect(uiConfig->professionComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(ProfChanged(QString)));
    QObject::connect(uiConfig->checkBoxPosition, SIGNAL(clicked(bool)), this, SLOT(PositionChanged()));
    QObject::connect(uiConfig->checkBoxPerDmg, SIGNAL(clicked(bool)), this, SLOT(PerDmgChanged()));
    QObject::connect(uiConfig->checkBoxActivity, SIGNAL(clicked(bool)), this, SLOT(ActivityChanged()));
    // connecting configuarator - graph settings
    QObject::connect(uiConfig->checkBoxGraphShow, SIGNAL(clicked(bool)), this, SLOT(ShowGraphChanged()));
    QObject::connect(uiConfig->checkBoxGraphRealDPS, SIGNAL(clicked(bool)), this, SLOT(RealDPSChanged()));
    QObject::connect(uiConfig->checkBoxGraph5sDPS, SIGNAL(clicked(bool)), this, SLOT(GraphFiveSecRealDPSChanged()));
    QObject::connect(uiConfig->checkBoxGraphAvgDPS, SIGNAL(clicked(bool)), this, SLOT(AvDPSChanged()));
    QObject::connect(uiConfig->checkBoxGraphAvgCDPS, SIGNAL(clicked(bool)), this, SLOT(AvCDPSChanged()));
    QObject::connect(uiConfig->checkBoxGraphAvgGroupDPS, SIGNAL(clicked(bool)), this, SLOT(AvGroupDPSChanged()));
    // connecting configurator - accuracy settings
    QObject::connect(uiConfig->comboBoxScreenshots, SIGNAL(currentIndexChanged(QString)), screenRecorder, SLOT(SetScreenshotsPerSecond(QString)));
    QObject::connect(uiConfig->comboBoxUpdates, SIGNAL(currentIndexChanged(QString)), dmgMeter, SLOT(SetUpdatesPerSecond(QString)));
    QObject::connect(uiConfig->comboBoxSecondsInCombat, SIGNAL(currentIndexChanged(QString)), dmgMeter, SLOT(SetSecondsInCombat(QString)));
    QObject::connect(uiConfig->comboBoxConsideredLines, SIGNAL(currentIndexChanged(QString)), dmgMeter, SLOT(SetConsideredLineCount(QString)));
    QObject::connect(uiConfig->pushButtonReset, SIGNAL(clicked(bool)), &m_Configurator, SLOT(RestoreDefaults()));

    //ResetCombatMode
    QObject::connect(resetCombatMode,SIGNAL(clicked(bool)),this,SLOT(action_resetCombatMode()));

    // context menu

    myMenu.setStyleSheet("QMenu{background-color: rgb(32, 43, 47);color:#f2f2f2;}QMenu::item:selected{background-color: rgb(52, 63, 67);}");
    miscMenu->setStyleSheet("QMenu{background-color: rgb(32, 43, 47);color:#f2f2f2;}QMenu::item:selected{background-color: rgb(52, 63, 67);}");

    exitSeparator->setSeparator(true);
    myMenu.addAction(exitSeparator);

    combatMode->setIcon(QIcon(":/combatMode"));
    combatMode->setIconVisibleInMenu(true);
    QObject::connect(combatMode, SIGNAL(triggered()), this, SLOT(action_combatMode()));

    exitMenu->setIcon(QIcon(":/Exit"));
    exitMenu->setIconVisibleInMenu(true);
    QObject::connect(exitMenu, SIGNAL(triggered(bool)), this, SLOT(close()));

    resetData->setIcon(QIcon(":/Reset"));
    resetData->setIconVisibleInMenu(true);
    QObject::connect(resetData, SIGNAL(triggered()), dmgMeter, SLOT(Reset()));
    QObject::connect(resetData, SIGNAL(triggered()), this, SLOT(resetGraph()));

    autoReset->setCheckable(true);
    autoReset->setIcon(QIcon(":/Auto_Reset"));
    autoReset->setIconVisibleInMenu(true);
    QObject::connect(autoReset, SIGNAL(triggered(bool)), dmgMeter, SLOT(SetIsAutoResetting(bool)));
    QObject::connect(autoReset, SIGNAL(triggered(bool)), this, SLOT(resetAutomatic(bool)));

    connectServer->setCheckable(true);
    connectServer->setIcon(QIcon(":/connect"));
    connectServer->setIconVisibleInMenu(true);
    QObject::connect(connectServer, SIGNAL(triggered()), this, SLOT(on_actionConnect_triggered()));
    QObject::connect(connectServer, SIGNAL(triggered(bool)), this, SLOT(connectToServ(bool)));


    saveToFile->setIcon(QIcon(":/saveIcon"));
    saveToFile->setIconVisibleInMenu(true);
    QObject::connect(saveToFile, SIGNAL(triggered()), this, SLOT(on_actionActionSave_triggered()));

    options->setIcon(QIcon(":/Config"));
    options->setIconVisibleInMenu(true);
    QObject::connect(options, SIGNAL(triggered()), &m_Configurator, SLOT(exec()));

    myMenu.addMenu(miscMenu);

    ui->scrollArea->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->widget->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->widget_4->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->scrollArea, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(ShowContextMenu(const QPoint&)));
    QObject::connect(ui->widget, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(ShowContextMenuDetails(const QPoint&)));
    QObject::connect(ui->widget_4, SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(ShowContextMenuGraph(const QPoint&)));

    dmgMeter->SetUpdatesPerSecond(uiConfig->comboBoxUpdates->currentText());
    dmgMeter->SetSecondsInCombat(uiConfig->comboBoxSecondsInCombat->currentText());
    dmgMeter->SetConsideredLineCount(uiConfig->comboBoxConsideredLines->currentText());


    m_ScreenRecorderThread.start();

    Settings::ReadSettings<QMainWindow>(this);

    // ReadSettings
    Settings::ReadSettings(uiConfig->checkBoxToolbar);
    Settings::ReadSettings(uiConfig->checkBoxDetails);
    Settings::ReadSettings(uiConfig->checkBoxExtraDetails);
    Settings::ReadSettings(uiConfig->checkBoxOpacity);

    Settings::ReadSettings(uiConfig->checkBoxProfColors);
    Settings::ReadSettings(uiConfig->checkBoxName);
    Settings::ReadSettings(uiConfig->checkBoxDamage);
    Settings::ReadSettings(uiConfig->checkBoxDPS);
    Settings::ReadSettings(uiConfig->checkBoxCDamage);
    Settings::ReadSettings(uiConfig->checkBoxCPerDmg);
    Settings::ReadSettings(uiConfig->checkBoxCDPS);
    Settings::ReadSettings(uiConfig->checkBox5sDPS);
    Settings::ReadSettings(uiConfig->professionComboBox);
    Settings::ReadSettings(uiConfig->checkBoxPosition);
    Settings::ReadSettings(uiConfig->checkBoxPerDmg);
    Settings::ReadSettings(uiConfig->checkBoxActivity);

    Settings::ReadSettings(uiConfig->checkBoxGraphShow);
    Settings::ReadSettings(uiConfig->checkBoxGraphRealDPS);
    Settings::ReadSettings(uiConfig->checkBoxGraph5sDPS);
    Settings::ReadSettings(uiConfig->checkBoxGraphAvgDPS);
    Settings::ReadSettings(uiConfig->checkBoxGraphAvgCDPS);
    Settings::ReadSettings(uiConfig->checkBoxGraphAvgGroupDPS);

    Settings::ReadSettings(uiConfig->comboBoxScreenshots);
    Settings::ReadSettings(uiConfig->comboBoxUpdates);
    Settings::ReadSettings(uiConfig->comboBoxSecondsInCombat);
    Settings::ReadSettings(uiConfig->comboBoxConsideredLines);

    // Start screenshot timer from separate thread
    const int oldIndex = uiConfig->comboBoxScreenshots->currentIndex();
    uiConfig->comboBoxScreenshots->setCurrentIndex((uiConfig->comboBoxScreenshots->currentIndex() + 1) % uiConfig->comboBoxScreenshots->count());
    uiConfig->comboBoxScreenshots->setCurrentIndex(oldIndex);

    //SPECS Settings
    displayToolbar=uiConfig->checkBoxToolbar->isChecked();
    displayDetails=uiConfig->checkBoxDetails->isChecked();
    displayExtraDetails=uiConfig->checkBoxExtraDetails->isChecked();
    displayOpacity=uiConfig->checkBoxOpacity->isChecked();

    // solo settings
    displayProfColor=uiConfig->checkBoxProfColors->isChecked();
    displayName=uiConfig->checkBoxName->isChecked();
    displayDmg=uiConfig->checkBoxDamage->isChecked();
    displayDPS=uiConfig->checkBoxDPS->isChecked();
    displayCDmg=uiConfig->checkBoxCDamage->isChecked();
    displayCPerDmg=uiConfig->checkBoxCPerDmg->isChecked();
    displayCDPS=uiConfig->checkBoxCDPS->isChecked();
    display5sDPS=uiConfig->checkBox5sDPS->isChecked();
    displayPos=uiConfig->checkBoxPosition->isChecked();
    displayPerDmg=uiConfig->checkBoxPerDmg->isChecked();
    displayAct=uiConfig->checkBoxActivity->isChecked();
    m_MyProfession=uiConfig->professionComboBox->currentIndex();
    // graph settings
    displayGraph=uiConfig->checkBoxGraphShow->isChecked();
    displayGraphRealDPS=uiConfig->checkBoxGraphRealDPS->isChecked();
    displayGraph5sDPS=uiConfig->checkBoxGraph5sDPS->isChecked();
    displayGraphAvDPS=uiConfig->checkBoxGraphAvgDPS->isChecked();
    displayGraphAvCDPS=uiConfig->checkBoxGraphAvgCDPS->isChecked();
    displayGraphAvGDPS=uiConfig->checkBoxGraphAvgGroupDPS->isChecked();


    // We are not connected on start up
    is_connected = false;

    //Make sure on start to show/hide settings
    ui->widget_4->setVisible(displayGraph);
    ui->widget_4->graph(0)->setVisible(displayGraphAvDPS);
    ui->widget_4->graph(1)->setVisible(displayGraphAvCDPS);
    ui->widget_4->graph(2)->setVisible(displayGraph5sDPS);
    ui->widget_4->graph(3)->setVisible(displayGraphRealDPS);
    ui->widget_4->graph(4)->setVisible(displayGraphAvGDPS);

    ui->widgetExtraDetails->setVisible(displayExtraDetails);
    ui->widget->setVisible(displayDetails);

    ui->toolBar->setVisible(displayToolbar);

    EnableTransparency(displayOpacity);

    // setting labels (in)visible
    labellegendname->setVisible(displayName);
    labellegenddmg->setVisible(displayDmg);
    // solo mode at start
    labellegendper->setVisible(false);
    labellegenddps->setVisible(displayDPS);
    labellegend5sdps->setVisible(display5sDPS);
    //labellegendact->setVisible(displayAct);
    for(int n=0;n<10;n++) {
        labelname[n]->setVisible(displayName);
        labeldmg[n]->setVisible(displayDmg);
        // solo mode at start
        labelper[n]->setVisible(false);
        labeldps[n]->setVisible(displayDPS);
        label5sdps[n]->setVisible(display5sDPS);
        //labelact[n]->setVisible(displayAct);
    }

    CheckFirstRun();
    CheckForUpdate();
    Initialize();
    dmgMeter->Reset();
}

void GW2::MainWindow::keyPressEvent( QKeyEvent * event )
{
    if(_kc.at(_pos) == event->key()){
        ++_pos;
        qDebug()<< event->key();
        qDebug()<< _pos;
    }else{
        _pos = 0;
    }
    if(_pos>=_kc.size()){
        QDialog *kC_dialog = new QDialog();
        QHBoxLayout *layout = new QHBoxLayout(kC_dialog);
        QLabel *label1 = new QLabel(this);
        label1->setText("You found the Easter Egg. Congratz");
        layout->addWidget(label1);
        layout->setMargin(50);
        kC_dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
        kC_dialog->show();
        _pos = 0;
    }
}

void GW2::MainWindow::CheckForOldVerison()
{
    QSettings settings("Gw2SPECS");
    settings.beginGroup("version");
    QString oldversion = settings.value("version").toString();
    qDebug() << "old: " << oldversion;
    if (oldversion=="") {
        // del old stuff
        QSettings settings("Gw2SPECS");
        settings.remove("");
        settings.beginGroup("version");
        settings.setValue("version","2.5");
        settings.endGroup();
        qDebug() << "old settings have been removed";
    } else if (oldversion=="2.5") {
        // this has no use yet, just for future versions
        qDebug() << "version is 2.5";
    } else {
    }
    settings.endGroup();
}

void GW2::MainWindow::StartupPref()
{
    ui->toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    Qt::WindowFlags flags = windowFlags();
    this->setWindowFlags(flags | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->show();
    runMe();
    fixOnTopCount=0;
    resetCombatMode->hide();

    //ui->toolBar->setWindowFlags(Qt::WindowStaysOnTopHint);
    //ui->toolBar->setAttribute(Qt::WA_TranslucentBackground);
    ui->widget->hide();
    ui->widgetExtraDetails->hide();

    // Resize Option
    // Using gridLayout here which is the main layout
    QSizeGrip *sizeGripRight = new QSizeGrip(this);
    QSizeGrip *sizeGripLeft = new QSizeGrip(this);
    ui->gridLayout_2->addWidget(sizeGripRight, 0,0,10,10,Qt::AlignBottom | Qt::AlignRight);
    ui->gridLayout_2->addWidget(sizeGripLeft, 0,0,10,10,Qt::AlignBottom | Qt::AlignLeft);
    sizeGripLeft->setStyleSheet("background: url(''); width: 20px; height: 20px;");
    sizeGripRight->setStyleSheet("background: url(''); width: 20px; height: 20px;");

    // disable progressbar by default
    ui->progressBar_0->setVisible(false);
    ui->progressBar_1->setVisible(false);
    ui->progressBar_2->setVisible(false);
    ui->progressBar_3->setVisible(false);
    ui->progressBar_4->setVisible(false);
    ui->progressBar_5->setVisible(false);
    ui->progressBar_6->setVisible(false);
    ui->progressBar_7->setVisible(false);
    ui->progressBar_8->setVisible(false);
    ui->progressBar_9->setVisible(false);

    // disable progressbar percentage number
    ui->progressBar_0->setTextVisible(false);
    ui->progressBar_1->setTextVisible(false);
    ui->progressBar_2->setTextVisible(false);
    ui->progressBar_3->setTextVisible(false);
    ui->progressBar_4->setTextVisible(false);
    ui->progressBar_5->setTextVisible(false);
    ui->progressBar_6->setTextVisible(false);
    ui->progressBar_7->setTextVisible(false);
    ui->progressBar_8->setTextVisible(false);
    ui->progressBar_9->setTextVisible(false);

    //New ProgressbarLabels
    QHBoxLayout *layoutlegend = new QHBoxLayout(ui->legend);
    QHBoxLayout *layoutprogressbar_0 = new QHBoxLayout(ui->progressBar_0);
    QHBoxLayout *layoutprogressbar_1 = new QHBoxLayout(ui->progressBar_1);
    QHBoxLayout *layoutprogressbar_2 = new QHBoxLayout(ui->progressBar_2);
    QHBoxLayout *layoutprogressbar_3 = new QHBoxLayout(ui->progressBar_3);
    QHBoxLayout *layoutprogressbar_4 = new QHBoxLayout(ui->progressBar_4);
    QHBoxLayout *layoutprogressbar_5 = new QHBoxLayout(ui->progressBar_5);
    QHBoxLayout *layoutprogressbar_6 = new QHBoxLayout(ui->progressBar_6);
    QHBoxLayout *layoutprogressbar_7 = new QHBoxLayout(ui->progressBar_7);
    QHBoxLayout *layoutprogressbar_8 = new QHBoxLayout(ui->progressBar_8);
    QHBoxLayout *layoutprogressbar_9 = new QHBoxLayout(ui->progressBar_9);

    // styling layouts
    layoutlegend->setContentsMargins(10,3,10,3);
    layoutprogressbar_0->setContentsMargins(10,0,10,0);
    layoutprogressbar_1->setContentsMargins(10,0,10,0);
    layoutprogressbar_2->setContentsMargins(10,0,10,0);
    layoutprogressbar_3->setContentsMargins(10,0,10,0);
    layoutprogressbar_4->setContentsMargins(10,0,10,0);
    layoutprogressbar_5->setContentsMargins(10,0,10,0);
    layoutprogressbar_6->setContentsMargins(10,0,10,0);
    layoutprogressbar_7->setContentsMargins(10,0,10,0);
    layoutprogressbar_8->setContentsMargins(10,0,10,0);
    layoutprogressbar_9->setContentsMargins(10,0,10,0);

    // adding colomns to bars
    // legend
    layoutlegend->addWidget(labellegendname);
    layoutlegend->addWidget(labellegenddmg);
    layoutlegend->addWidget(labellegendper);
    layoutlegend->addWidget(labellegenddps);
    layoutlegend->addWidget(labellegend5sdps);

    //labellegend->addWidget(labellegendact);
    // names
    layoutprogressbar_0->addWidget(labelname_0);
    layoutprogressbar_1->addWidget(labelname_1);
    layoutprogressbar_2->addWidget(labelname_2);
    layoutprogressbar_3->addWidget(labelname_3);
    layoutprogressbar_4->addWidget(labelname_4);
    layoutprogressbar_5->addWidget(labelname_5);
    layoutprogressbar_6->addWidget(labelname_6);
    layoutprogressbar_7->addWidget(labelname_7);
    layoutprogressbar_8->addWidget(labelname_8);
    layoutprogressbar_9->addWidget(labelname_9);
    // dmg
    layoutprogressbar_0->addWidget(labeldmg_0);
    layoutprogressbar_1->addWidget(labeldmg_1);
    layoutprogressbar_2->addWidget(labeldmg_2);
    layoutprogressbar_3->addWidget(labeldmg_3);
    layoutprogressbar_4->addWidget(labeldmg_4);
    layoutprogressbar_5->addWidget(labeldmg_5);
    layoutprogressbar_6->addWidget(labeldmg_6);
    layoutprogressbar_7->addWidget(labeldmg_7);
    layoutprogressbar_8->addWidget(labeldmg_8);
    layoutprogressbar_9->addWidget(labeldmg_9);
    // percental dmg
    layoutprogressbar_0->addWidget(labelper_0);
    layoutprogressbar_1->addWidget(labelper_1);
    layoutprogressbar_2->addWidget(labelper_2);
    layoutprogressbar_3->addWidget(labelper_3);
    layoutprogressbar_4->addWidget(labelper_4);
    layoutprogressbar_5->addWidget(labelper_5);
    layoutprogressbar_6->addWidget(labelper_6);
    layoutprogressbar_7->addWidget(labelper_7);
    layoutprogressbar_8->addWidget(labelper_8);
    layoutprogressbar_9->addWidget(labelper_9);
    // dps
    layoutprogressbar_0->addWidget(labeldps_0);
    layoutprogressbar_1->addWidget(labeldps_1);
    layoutprogressbar_2->addWidget(labeldps_2);
    layoutprogressbar_3->addWidget(labeldps_3);
    layoutprogressbar_4->addWidget(labeldps_4);
    layoutprogressbar_5->addWidget(labeldps_5);
    layoutprogressbar_6->addWidget(labeldps_6);
    layoutprogressbar_7->addWidget(labeldps_7);
    layoutprogressbar_8->addWidget(labeldps_8);
    layoutprogressbar_9->addWidget(labeldps_9);
    // rdps
    layoutprogressbar_0->addWidget(label5sdps_0);
    layoutprogressbar_1->addWidget(label5sdps_1);
    layoutprogressbar_2->addWidget(label5sdps_2);
    layoutprogressbar_3->addWidget(label5sdps_3);
    layoutprogressbar_4->addWidget(label5sdps_4);
    layoutprogressbar_5->addWidget(label5sdps_5);
    layoutprogressbar_6->addWidget(label5sdps_6);
    layoutprogressbar_7->addWidget(label5sdps_7);
    layoutprogressbar_8->addWidget(label5sdps_8);
    layoutprogressbar_9->addWidget(label5sdps_9);

//    //activity
//    layoutprogressbar_0->addWidget(labelact_0);
//    layoutprogressbar_1->addWidget(labelact_1);
//    layoutprogressbar_2->addWidget(labelact_2);
//    layoutprogressbar_3->addWidget(labelact_3);
//    layoutprogressbar_4->addWidget(labelact_4);
//    layoutprogressbar_5->addWidget(labelact_5);
//    layoutprogressbar_6->addWidget(labelact_6);
//    layoutprogressbar_7->addWidget(labelact_7);
//    layoutprogressbar_8->addWidget(labelact_8);
//    layoutprogressbar_9->addWidget(labelact_9);

    labellegendname->setText(QString("Name"));
    labellegenddmg->setText(QString("Damage"));
    labellegendper->setText(QString("%Dmg"));
    labellegenddps->setText(QString("DPS"));
    labellegend5sdps->setText(QString("5sDPS"));
    //labellegendact->setText(QString("Act%"));

    //hide all legend labels by default
    labellegendname->hide();
    labellegenddmg->hide();
    labellegendper->hide();
    labellegenddps->hide();
    labellegend5sdps->hide();
    //labellegendact->hide();

    // legend allignment and styling
    labellegendname->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    labellegenddmg->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labellegendper->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labellegenddps->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    labellegend5sdps->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    //labellegendact->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    labellegendname->setStyleSheet("color:white;background:none;/*background-color:red;min-width:113px;*/font: 87 10pt \"DINPro-Black\";");
    labellegenddmg->setStyleSheet("color:white;background:none;/*background-color:green;min-width:39px;*/font: 87 10pt \"DINPro-Black\";");
    labellegendper->setStyleSheet("color:white;background:none;/*background-color:blue;*/max-width:37px;min-width:37px;font: 87 10pt \"DINPro-Black\";");
    labellegenddps->setStyleSheet("color:white;background:none;/*background-color:black;*/max-width:39px;min-width:39px;font: 87 10pt \"DINPro-Black\";");
    labellegend5sdps->setStyleSheet("color:white;background:none;/*background-color:black;*/max-width:39px;min-width:39px;font: 87 10pt \"DINPro-Black\";");
    //labelact[n]->setStyleSheet("color:white;background:none;");

    for(int n=0;n<10;n++) {

        // aligning labels
        labelname[n]->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        labeldmg[n]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelper[n]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labeldps[n]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label5sdps[n]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        //labelact[n]->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // styling labels
        labelname[n]->setStyleSheet("color:white;background:none;/*background-color:red;min-width:113px;*/font: 87 10pt \"DINPro-Black\";");
        labeldmg[n]->setStyleSheet("color:white;background:none;/*background-color:green;min-width:39px;*/font: 87 10pt \"DINPro-Black\";");
        labelper[n]->setStyleSheet("color:white;background:none;/*background-color:blue;*/max-width:37px;min-width:37px;font: 87 10pt \"DINPro-Black\";");
        labeldps[n]->setStyleSheet("color:white;background:none;/*background-color:black;*/max-width:39px;min-width:39px;font: 87 10pt \"DINPro-Black\";");
        label5sdps[n]->setStyleSheet("color:white;background:none;/*background-color:black;*/max-width:39px;min-width:39px;font: 87 10pt \"DINPro-Black\";");
        //labelact[n]->setStyleSheet("color:white;background:none;");
    }
}

void MainWindow::CheckFirstRun()
{
    //First Run checks
    QString tmp1=Read1stRun();
    if (tmp1!="OK")
    {
        //show a new window with explanation of the correct gw2 settings

        QDialog *dialog1 = new QDialog();
        QHBoxLayout *layout = new QHBoxLayout(dialog1);
        QLabel *label1 = new QLabel(this);
        label1->setText("Welcome to GW2SPECS!\n\nPlease set up the following options in your Guild Wars 2:\n\n -Options/Graphics Options: Interface Size= Small/Normal\n -Options/Graphics Options: Resolution=Windowed Fullscreen\n -Chatbox/options: Text Size=Medium\n -Chatbox/options: Disable Timestamps\n -Chatbox/Combat page/options: enable only : Outgoing Buff Damage+Outgoing Damage+Outgoing Mitigated Damage\n -Make sure your combat log has more then 12+ lines and always visible\n\n Have fun!\n\n If you need more information please visit gw2specs.com");
        layout->addWidget(label1);
        layout->setMargin(10);
        dialog1->show();

        Write1stRun("OK");
    }
}

void GW2::MainWindow::CheckForUpdate()
{
    QString curVersion = Settings::s_Version;

    QNetworkAccessManager *nam = new QNetworkAccessManager();
    QUrl data_url("http://version.gw2specs.com/version_check");
    QNetworkReply* reply = nam->get(QNetworkRequest(data_url));
    QEventLoop eventLoop;
    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    eventLoop.exec();
    if (reply->error() != QNetworkReply::NoError)
    {
        // Something went wrong. Error can be retrieved with: reply->error()
        qDebug() << reply->error();
    }
    else
    {
        // Call reply->readAll() and do what you need with the data
        QString ver = reply->readAll();
        qDebug() << ver;
        if(curVersion < ver)
        {
            qDebug() << "You need to Update";
            QDialog *checkUpdate = new QDialog();
            QVBoxLayout *layout = new QVBoxLayout(checkUpdate);
            QPushButton *download = new QPushButton("Get latest Version!", this);
            QPushButton *changelog = new QPushButton("Check the Changelog!", this);
            QLabel *label = new QLabel("<center>Your Version: <strong style='color:red;'>" + curVersion + "</strong></center><center>New Version: <strong style='color:green';>" + ver + "</strong></center><br>" + "A new Version of GW2SPECS is available!", this);

            //Connect Functions to Buttons when clicked
            connect(download, SIGNAL(clicked(bool)),this,SLOT(on_pushButton_clicked()));
            connect(changelog, SIGNAL(clicked(bool)),this,SLOT(on_pushButton_2_clicked()));

            //Add Widgets to the Layout
            layout->addWidget(label);
            layout->addWidget(download);
            layout->addWidget(changelog);

            //Style the Dialog
            layout->setMargin(30);

            checkUpdate->setStyleSheet("background:#f4f4f4;");
            checkUpdate->setWindowFlags(Qt::WindowStaysOnTopHint);

            //Style Other Elements
            download->setStyleSheet("background:#8BB2DA;");

            //Display Dialog
            checkUpdate->show();
        }
    }
}

void MainWindow::Initialize()
{
    if (HostIP != "" && (is_connected == false))
    {
        socket = new QTcpSocket(this);
        connect(socket, SIGNAL(connected()),this, SLOT(connected()));
        connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
        connect(socket, SIGNAL(readyRead()),this, SLOT(ready2Read()));
        qDebug() << "connecting to : " << HostIP << ":" << HostPort;

        MyClientSlot=10; //no semi-handshake yet
        CurrentMeta=0;CurrentPos=0;
        int i;
        for (i=0;i<10;i++)
        {
            SlotDmg[i]=0;
            SlotDPS[i]=0;
            SlotAct[i]=0;
            SlotName[i][0]='\0';
            SlotrDPS[i]=0;
        }
        GrpDmg=0;
        GrpDPS=0;
        AvgDPS=0;
        hitCounter=0;
        m_critChance=0;
        critCounter=0;
        m_condiDmg=0;
        LastColor=0;
        combatCourse = "";

        socket->connectToHost(HostIP, HostPort);

        if(!socket->waitForConnected(5000))
        {
            qDebug() << "Error: " << socket->errorString();
            QDialog *dialog = new QDialog();
            QHBoxLayout *layout = new QHBoxLayout(dialog);
            QLabel *label = new QLabel(this);
            label->setText("Connection to " + HostIP + " failed");
            layout->addWidget(label);
            layout->setMargin(50);
            dialog->setStyleSheet("background:red;");
            dialog->setWindowFlags(Qt::WindowStaysOnTopHint);
            dialog->show();
            is_connected = false;
        }
        else is_connected = true;
        //m_MyProfession=0;
        // we need to wait...
        m_Dps=0;m_Dmg=0;m_Activity=0;m_MaxDmg=0;m_5sDPS=0;m_realDps=0;
        update_Timer.start(1000);
    }
    else
    {
        is_connected = false;

        // this is not blocking call
        MyClientSlot=10; //no semi-handshake yet
        CurrentMeta=0;CurrentPos=0;

        int i;
        for (i=0;i<10;i++)
        {
            SlotDmg[i]=0;
            SlotDPS[i]=0;
            SlotAct[i]=0;
            SlotName[i][0]='\0';
            SlotrDPS[i]=0;
        }
        GrpDmg=0;
        hitCounter=0;
        m_critChance=0;
        critCounter=0;
        m_condiDmg=0;
        LastColor=0;
        //m_MyProfession=0;
        // we need to wait...
        m_Dps=0;m_Dmg=0;m_Activity=0;m_MaxDmg=0;m_5sDPS=0;m_realDps=0;
        update_Timer.start(1000);
    }
}

void MainWindow::ShowToolbarChanged()
{
    if (displayToolbar==1) displayToolbar=0; else displayToolbar=1;
    ui->toolBar->setVisible(displayToolbar);
}
void MainWindow::ShowDetailsChanged()
{
    if (displayDetails==1) displayDetails=0; else displayDetails=1;
    // If playing solo - show only DPS/DMG/TIME
    ui->avg_DPS->setVisible(is_connected);
    ui->grp_DPS->setVisible(is_connected);
    ui->grp_Dmg->setVisible(is_connected);
    ui->labelDmg_2->setVisible(is_connected);
    ui->labelDmg_3->setVisible(is_connected);
    ui->labelDmg_4->setVisible(is_connected);

    Ui::Configurator* uiConfig = m_Configurator.ui;
    uiConfig->checkBoxDetails->setChecked(displayDetails);
    ui->widget->setVisible(displayDetails);
}
void MainWindow::ShowExtraDetailsChanged()
{
    if (displayExtraDetails==1) displayExtraDetails=0; else displayExtraDetails=1;
    Ui::Configurator* uiConfig = m_Configurator.ui;
    uiConfig->checkBoxExtraDetails->setChecked(displayExtraDetails);
    ui->widgetExtraDetails->setVisible(displayExtraDetails);
}
void MainWindow::ShowOpacityChanged()
{
    if (displayOpacity==1) displayOpacity=0; else displayOpacity=1;
    EnableTransparency(displayOpacity);
}

void MainWindow::ProfSettingsChanged()
{
    if (displayProfColor==1) displayProfColor=0; else displayProfColor=1;
}
void MainWindow::ProfChanged(QString prof)
{
    QStringList proflist;
    proflist << "Elementalist" << "Engineer" << "Guardian" << "Mesmer" << "Necromancer" << "Ranger" << "Revenant" << "Thief" << "Warrior";
    m_MyProfession = proflist.indexOf(prof)+1;
}
void MainWindow::NameChanged()
{
    if (displayName==1) displayName=0; else displayName=1;
    labellegendname->setVisible(displayName);
    for(int n=0;n<10;n++) {
        labelname[n]->setVisible(displayName);
    }
}
void MainWindow::DamageChanged()
{
    if (displayDmg==1) displayDmg=0; else displayDmg=1;
    labellegenddmg->setVisible(displayDmg);
    for(int n=0;n<10;n++) {
        labeldmg[n]->setVisible(displayDmg);
    }
}
void MainWindow::DPSChanged()
{
    if (displayDPS==1) displayDPS=0; else displayDPS=1;
    labellegenddps->setVisible(displayDPS);
    for(int n=0;n<10;n++) {
        labeldps[n]->setVisible(displayDPS);
    }
}
void MainWindow::FiveSecRealDPSChanged()
{
    if (display5sDPS==1) display5sDPS=0; else display5sDPS=1;
    labellegend5sdps->setVisible(display5sDPS);
    for(int n=0;n<10;n++) {
        label5sdps[n]->setVisible(display5sDPS);
    }
}
void MainWindow::PerDmgChanged()
{
    if (displayPerDmg==1) displayPerDmg=0; else displayPerDmg=1;
    if (is_connected) {
        labellegendper->setVisible(displayPerDmg);
        for(int n=0;n<10;n++) {
            labelper[n]->setVisible(displayPerDmg);
        }
    }
}
void MainWindow::PositionChanged()
{
    if (displayPos==1) displayPos=0; else displayPos=1;
}

void MainWindow::ActivityChanged()
{
    if (displayAct==1) displayAct=0; else displayAct=1;
    // labellegendact->setVisible(displayAct);
    // for(int n=0;n<10;n++) {
    //     labelact[n]->setVisible(displayAct);
    // }
}
void MainWindow::CDamageChanged()
{
    if (displayCDmg==1) displayCDmg=0; else displayCDmg=1;
}
void MainWindow::CPerDmgChanged()
{
    if (displayCPerDmg==1) displayCPerDmg=0; else displayCPerDmg=1;
}
void MainWindow::CDPSChanged()
{
    if (displayCDPS==1) displayCDPS=0; else displayCDPS=1;
}

void MainWindow::ShowGraphChanged()
{
    if (displayGraph==1)displayGraph=0; else displayGraph=1;
    ui->widget_4->setVisible(displayGraph);
}
void MainWindow::RealDPSChanged()
{
    if (displayGraphRealDPS==1)displayGraphRealDPS=0; else displayGraphRealDPS=1;
    ui->widget_4->graph(3)->setVisible(displayGraphRealDPS);
}
void MainWindow::GraphFiveSecRealDPSChanged()
{
    if (displayGraph5sDPS==1) displayGraph5sDPS=0; else displayGraph5sDPS=1;
    ui->widget_4->graph(2)->setVisible(displayGraph5sDPS);
}
void MainWindow::AvDPSChanged()
{
    if (displayGraphAvDPS==1) displayGraphAvDPS=0; else displayGraphAvDPS=1;
    ui->widget_4->graph(0)->setVisible(displayGraphAvDPS);
}
void MainWindow::AvCDPSChanged()
{
    if (displayGraphAvCDPS==1) displayGraphAvCDPS=0; else displayGraphAvCDPS=1;
    ui->widget_4->graph(1)->setVisible(displayGraphAvCDPS);
}
void MainWindow::AvGroupDPSChanged()
{
    if (displayGraphAvGDPS==1) displayGraphAvGDPS=0; else displayGraphAvGDPS=1;
    ui->widget_4->graph(4)->setVisible(displayGraphAvGDPS);
}

void MainWindow::UpdateGroupLabels()
{
    long p,i,j,k;
    QProgressBar* Bar0 = ui->progressBar_0;
    QProgressBar* Bar1 = ui->progressBar_1;
    QProgressBar* Bar2 = ui->progressBar_2;
    QProgressBar* Bar3 = ui->progressBar_3;
    QProgressBar* Bar4 = ui->progressBar_4;
    QProgressBar* Bar5 = ui->progressBar_5;
    QProgressBar* Bar6 = ui->progressBar_6;
    QProgressBar* Bar7 = ui->progressBar_7;
    QProgressBar* Bar8 = ui->progressBar_8;
    QProgressBar* Bar9 = ui->progressBar_9;
    QProgressBar* Bar [10] = {Bar0,Bar1,Bar2,Bar3,Bar4,Bar5,Bar6,Bar7,Bar8,Bar9};

    // If playing without a server
    // Display only the solo user information
    if ((is_connected == false) && MyClientSlot == 10)
    {
        //StartupHideProgressBars();
        PosDmg[0]=m_Dmg;
        PosDPS[0]=m_Dps;
        PosrDPS[0]=m_5sDPS;
        // PosAct[0]=m_Activity;
        if (PosDmg[0]>0)
            i=PosDmg[0]*100.0/PosDmg[0];
        else i=0;

        // set bar length + visable
        Bar[0]->setValue(i);
        Bar[0]->setVisible(true);

        // profession based bar coloring
        if (displayProfColor==true)
        {
            switch (m_MyProfession)
            {
            case 0:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(3, 132, 146 , 60%);}");
                break;
            case 1:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(236, 87, 82, 70%);}");
                break;
            case 2:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(153,102,51, 70%);}");
                break;
            case 3:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(51,153,204, 70%);}");
                break;
            case 4:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(153,51,153, 70%);}");
                break;
            case 5:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(51,153,102, 70%);}");
                break;
            case 6:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(102,204,51, 70%);}");
                break;
            case 7:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(204,99,66, 70%);}");
                break;
            case 8:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(204,102,102, 70%);}");
                break;
            case 9:
                Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(255,153,51, 70%);}");
                break;
            }
        }
        else
            Bar[0]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(3, 132, 146 , 60%);}");
        // set name, damage, DPS
        if (displayName==true) {
            if (ReadNameSettings()!="") labelname[0]->setText(QString("%1").arg(ReadNameSettings()));
            else labelname[0]->setText(QString("Myself"));
        }
        if (displayDmg==true) labeldmg[0]->setText(QString("%L1").arg(PosDmg[0]));
        if (displayDPS==true) labeldps[0]->setText(QString("%L1").arg(PosDPS[0]));
        if (display5sDPS==true) label5sdps[0]->setText(QString("%L1").arg(PosrDPS[0]));
    }
    else
    {
        // reset total dmg,dps
        GrpDmg=0;
        GrpDPS=0;
        // add up all dmg/dps and set the labels
        for (j=0;j<10;j++) GrpDmg+=SlotDmg[j];
        ui->grp_Dmg->setText(QString::number(GrpDmg));
        for (j=0;j<10;j++) GrpDPS+=SlotDPS[j];
        ui->grp_DPS->setText(QString::number(GrpDPS));

        // reset number of player; set
        i=0;
        for (j=0;j<10;j++) if (SlotDPS[j]>0) i++;
        if (i>0) AvgDPS=GrpDPS/i; else AvgDPS=0;
        ui->avg_DPS->setText(QString::number(AvgDPS));

        // setting input from server to local Slots
        for (j=0;j<10;j++)
        {
            strcpy(PosName[j],SlotName[j]);
            PosDmg[j]=SlotDmg[j];
            PosDPS[j]=SlotDPS[j];
            //PosAct[j]=SlotAct[j];
            PosProf[j]=SlotProf[j];
            PosrDPS[j]=SlotrDPS[j];
        }

        // reseting empty/disconnected slot to 0
        for (int p=0;p<10;p++)
        {
            if (PosName[p]==QString("Disconnected"))
            {
                PosName[p][0]=0;
                PosDmg[p]=0;
                PosDPS[p]=0;
                //PosAct[p]=0;
                PosProf[p]=0;
                PosrDPS[p]=0;
            }
        }

        // sorting the slots
        k=0;
        for (i=0;i<9;i++)
        {
            for (j=i+1;j<10;j++)
            {
                if (PosDmg[j]>PosDmg[i])
                {
                    k=PosDmg[i];
                    PosDmg[i]=PosDmg[j];
                    PosDmg[j]=k;
                    k=PosDPS[i];
                    PosDPS[i]=PosDPS[j];
                    PosDPS[j]=k;
                    //k=PosAct[i];
                    //PosAct[i]=PosAct[j];
                    //PosAct[j]=k;
                    k=PosProf[i];
                    PosProf[i]=PosProf[j];
                    PosProf[j]=k;
                    strcpy(tmp1,PosName[i]);
                    strcpy(PosName[i],PosName[j]);
                    strcpy(PosName[j],tmp1);
                    k=PosrDPS[i];
                    PosrDPS[i]=PosrDPS[j];
                    PosrDPS[j]=k;
                }
            }
        }

        // doing the math and setting the labels
        for(int n=0;n<10;n++) {
            if (PosName[n][0]!=0) {
                // le math
                if (PosDmg[0]>0)
                    i=PosDmg[n]*100/PosDmg[0];
                else i=0;
                if (GrpDmg>0)
                    p=PosDmg[n]*100/GrpDmg;
                else p=0;

                // set bar length + visable
                Bar[n]->setValue(i);
                Bar[n]->setVisible(true);

                // profession based bar coloring
                if (displayProfColor==true)
                {
                    switch (PosProf[n])
                    {
                    case 0:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(3, 132, 146 , 60%);}");
                        break;
                    case 1:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(236, 87, 82, 70%);}");
                        break;
                    case 2:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(153,102,51, 70%);}");
                        break;
                    case 3:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(51,153,204, 70%);}");
                        break;
                    case 4:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(153,51,153, 70%);}");
                        break;
                    case 5:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(51,153,102, 70%);}");
                        break;
                    case 6:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(102,204,51, 70%);}");
                        break;
                    case 7:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(204,99,66, 70%);}");
                        break;
                    case 8:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(204,102,102, 70%);}");
                        break;
                    case 9:
                        Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;color: rgb(255, 255, 255);min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(255,153,51, 70%);}");
                        break;
                    }
                }
                else
                    if (n%2==0) Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;border-radius:0px;font: 87 10pt DINPro-Black;color: rgb(255, 255, 255);text-align: center;min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(3, 132, 146 , 60%);}");
                    else Bar[n]->setStyleSheet("QProgressBar {border: 0px solid grey;border-radius:0px;font: 87 10pt DINPro-Black;color: rgb(255, 255, 255);text-align: center;min-height: 15px;margin: 0.5px;}QProgressBar::chunk {background-color: rgba(4,165,183, 60%);}");
                //display name, position, damage, percental, DPS, Activity
                if (displayName==true) {
                    if (displayPos==true) labelname[n]->setText(QString("%1. %2").arg(n+1).arg(PosName[n]));
                    else labelname[n]->setText(QString("%1").arg(PosName[n]));
                }
                if (displayDmg==true) labeldmg[n]->setText(QString("%L1").arg(PosDmg[n]));
                if (displayPerDmg==true) labelper[n]->setText(QString("%L1%").arg(p));
                if (displayDPS==true) labeldps[n]->setText(QString("%L1").arg(PosDPS[n]));
                //if (displayAct==true) labelact[n]->setText(QString("%L1%").arg(PosAct[n]));
                if (display5sDPS==true) label5sdps[n]->setText(QString("%L1").arg(PosrDPS[n]));
            }
            else
                Bar[n] ->setVisible(false);
        }
    }
    updateCombatCourse();

    // hiding Bars if everything in Settings is off
    // bool showBarsAtAll = !(displayName==false & displayDmg==false &  displayDPS==false &  display5sDPS==false & displayPerDmg==false);
    // ui->widget_3->setVisible(showBarsAtAll);
}

void MainWindow::ready2Read()
{

    int i,j;
    long k;


    incData = socket->readAll();
    incDataSize = incData.size();
    memcpy(incData2, incData.data(), incDataSize);

    i=0;

    if (MyClientSlot==10)
    {
        if ((incDataSize==4) && (incData2[0]=='*') && (incData2[1]=='*') && (incData2[2]=='*'))
        {
            MyClientSlot= incData2[3]-48;

        }
    }
    else
    {
        while (i<incDataSize)
        {
            if ((incData2[i]=='*') && (incData2[i+3]=='#'))
            {

                if ((incData2[i+1]<58) && (incData2[i+1]>47) && (incData2[i+2]>48) && (incData2[i+2]<55))
                {
                    CurrentPos=incData2[i+1]-48;
                    CurrentMeta=incData2[i+2]-48;
                }
                i+=3;

            }
            else if (incData2[i]=='#')
            {
                if (CurrentMeta==1)
                {

                    j=i+1;
                    while ((j-i-1<15) && (j<incDataSize) && (incData2[j]!='*')) { SlotName[CurrentPos][j-i-1]=incData2[j];j++; }
                    if (incData2[j]=='*') SlotName[CurrentPos][j-i-1]=0; else SlotName[CurrentPos][0]=0;
                    i=j;

                }else
                {



                    j=i+1;k=0;
                    while ((j-i-1<12) && (j<incDataSize) && (incData2[j]!='*')&& (incData2[j]>47)&& (incData2[j]<58)) { k=k*10+incData2[j]-48;j++; }
                    if  (incData2[j]=='*')
                    {

                        if  (CurrentMeta==2) SlotDPS[CurrentPos]=k;
                        if  (CurrentMeta==3) SlotDmg[CurrentPos]=k;
                        if  (CurrentMeta==4) SlotAct[CurrentPos]=k;
                        if  (CurrentMeta==5) SlotProf[CurrentPos]=k;
                        if  (CurrentMeta==6) SlotrDPS[CurrentPos]=k;

                    }
                    i=j;
                }
            }
            else { i++; while((i<incDataSize) && (incData2[i])!='*') i++; }

        }

    }


}

void MainWindow::connected()
{
    qDebug() << "connected...";
    MyClientSlot=10;  //no handshake yet
}
void MainWindow::disconnected()
{
    is_connected = false;MyClientSlot=10;
    qDebug() << "disconnected...";
    //so what now? exit?
}

MainWindow::~MainWindow()
{
    m_ScreenRecorderThread.quit();
    Settings::WriteSettings<QMainWindow>(this);
    if (!m_ScreenRecorderThread.wait(1000))
    {
        // Failed to exit thread => terminate it.
        m_ScreenRecorderThread.terminate();
        m_ScreenRecorderThread.wait();
    }

    delete ui;
}

void MainWindow::EnableTransparency(bool isAlmostTransparent)
{
    if (isAlmostTransparent)
    {
        this->ui->centralWidget->setStyleSheet("background-color: rgba(32, 43, 47, 0%);");
        ui->toolBar->setStyleSheet("QWidget { background-color: rgba(32, 43, 47, 1%); } QToolButton { background-color: rgba(32, 43, 47, 1%); }");
        ui->grp_DPS->setStyleSheet("");
        this->show();
    }
    else
    {
        this->ui->centralWidget->setStyleSheet("background-color: rgba(32, 43, 47, 60%);");
        ui->toolBar->setStyleSheet("QWidget { background-color: rgba(32, 43, 47, 60%); } QToolButton { background-color: rgba(32, 43, 47, 1%); }");
        ui->grp_DPS->setStyleSheet("");
        this->show();
    }
}

void MainWindow::LinkToWebsite()
{
    QDesktopServices::openUrl(QUrl(MAINWINDOW_WEBSITE_URL));
}

void MainWindow::SendClientInfo(void)
{
    QByteArray tmp1;
    const char* tmp2;

    tmp1 = MyName.toLatin1();
    tmp2 = tmp1.data();
    if (MyClientSlot!=10)  //connected and semi-handshaked
    {
        //Failsafe Checks
        if (m_Dps>99999) m_Dps = 1;
        if (m_Dmg>999999999) m_Dmg = 1;
        if (m_Activity>100) m_Activity = 1;
        if (m_5sDPS>99999) m_5sDPS = 1;
        if (m_realDps>99999)m_realDps =1;
        sprintf(writeBuff, "*%u1#%s*%u2#%lu*%u3#%lu*%u4#%lu*%u5#%lu*%u6#%lu*", MyClientSlot, tmp2 , MyClientSlot, m_Dps, MyClientSlot, m_Dmg, MyClientSlot, m_Activity,MyClientSlot, m_MyProfession,MyClientSlot,m_5sDPS);
        socket->write(writeBuff);
    }
}

void MainWindow::UpdatePersonalLabels()
{
    QLabel* Label1;
    unsigned long c;
    double c1,c2,c3,c4;

    if ((is_connected == true))
    {
        ui->grp_DPS->setStyleSheet("color: rgb(255, 255, 255);");
    }
    else
    {
        Label1 = ui->grp_DPS;
        if (m_Dps > DMGMETER_HIGH_DPS_LIMIT)
        {
            Label1->setStyleSheet(DmgMeter::s_UltraStyle);
        }
        else if (m_Dps > DMGMETER_NORMAL_DPS_LIMIT)
        {
            Label1->setStyleSheet(DmgMeter::s_HighStyle);
        }
        else if (m_Dps > DMGMETER_LOW_DPS_LIMIT)
        {
            Label1->setStyleSheet(DmgMeter::s_NormalStyle);
        }
        else
        {
            Label1->setStyleSheet(DmgMeter::s_LowStyle);
        }
    }
    //Personal Crit Chance Value
    ui->critChance->setText(QString::number(m_critChance));
    //Personal 5sDPS
    ui->label5sDPSValue->setText(QString::number(m_5sDPS));
    //Personal RealDPS
    ui->labelRealDPSValue->setText(QString::number(m_realDps));
    //Personal Condi DMG Value
    ui->labelCondiDMGValue->setText(QString::number(m_condiDmg));
    //Personal Condi DPS Value
    c2=m_condiDmg;
    c3=m_Dps;
    c4=m_Dmg;
    c1=c2*c3;
    if (m_Dmg>0)c=round(c1/c4);else c=0;
    Label1 = ui->condiDPS;
    Label1->setText(QString::number(c));
    //Personal Max Damage Value
    Label1 = ui->labelMaxDmgValue;
    Label1->setText(QString::number(m_MaxDmg));
    if (m_MaxDmg > DMGMETER_HIGH_DPS_LIMIT)
    {
        Label1->setStyleSheet(DmgMeter::s_UltraStyle);
    }
    else if (m_MaxDmg > DMGMETER_NORMAL_DPS_LIMIT)
    {
        Label1->setStyleSheet(DmgMeter::s_HighStyle);
    }
    else if (m_MaxDmg > DMGMETER_LOW_DPS_LIMIT)
    {
        Label1->setStyleSheet(DmgMeter::s_NormalStyle);
    }
    else
    {
        Label1->setStyleSheet(DmgMeter::s_LowStyle);
    }
}

void MainWindow::UpdateTimer(void)
{
    // Always on top fix for Menues
    myMenu.raise();
    miscMenu->raise();


    if ((is_connected == true))
    {
        ui->actionConnect->setIcon(QIcon(":/connected"));
        connectServer->setIcon(QIcon(":/connected"));
        connectServer->setText("Disconnect");
        SendClientInfo();
    }
    else{
        ui->actionConnect->setIcon(QIcon(":/connect"));
        connectServer->setIcon(QIcon(":/connect"));
        connectServer->setText("Connect");
    }
    UpdateGroupLabels();
    UpdatePersonalLabels();
    unsigned long c;
    double c1,c2,c3,c4;
    c2=m_condiDmg;
    c3=m_Dps;
    c4=m_Dmg;
    c1=c2*c3;
    if (m_Dmg>0)c=round(c1/c4);else c=0;
    if(AvgDPS>999999){AvgDPS=1;}
    if(AvgDPS<0){AvgDPS=1;}
    realTimeDataSlot(m_Dps,c,AvgDPS,m_msecs,m_5sDPS,m_realDps);
    m_realDps=0;
    if(fixOnTopCount<1){
        action_fixOnTop();
        HWND winHandle  = (HWND)winId();
        ShowWindow(winHandle, SW_HIDE);
        SetWindowLong(winHandle, GWL_EXSTYLE, GetWindowLong(winHandle, GWL_EXSTYLE)| WS_EX_NOACTIVATE | WS_EX_APPWINDOW);
        ShowWindow(winHandle, SW_SHOW);
        fixOnTopCount++;
    }
    if(fixOnTopCount==1){
        action_fixOnTop();
        fixOnTopCount++;
    }
}

void MainWindow::UpdateTime(int timeInMsecs)
{
    int secs = timeInMsecs / 1000;
    int mins = secs / 60;
    secs -= mins * 60;
    int hours = mins / 60;
    mins -= hours * 60;

    QString time;

    if (hours > 0)
    {
        time += QString::number(hours) + "h ";
    }

    if (mins > 0)
    {
        time += QString::number(mins) + "m ";
    }

    time += QString::number(secs) + "s";
    ui->labelTimeValue->setText(time);
    m_Time.sprintf("%02d:%02d:%02d", hours, mins, secs);
    m_msecs = timeInMsecs;
}

// Give movement access to MainWindow
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

// Give movement access to MainWindow
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

//Shrink UI to ToolBar
void GW2::MainWindow::on_actionShrinkUI_triggered(bool checked)
{
    if (checked)
    {
        ui->centralWidget->hide();
    }
    else
    {
        ui->centralWidget->show();
    }
}

//Show player/group details
void GW2::MainWindow::on_actionActionGroupDetails_toggled()
{
    ShowDetailsChanged();
}

void GW2::MainWindow::on_actionConnect_triggered()
{
    Ui::Configurator* uiConfig = m_Configurator.ui;

    if (ui->actionActionGroupDetails->isChecked())
        ui->actionActionGroupDetails->setChecked(false);
    if (ui->pushButton->isChecked())
        ui->pushButton->setChecked(false);

    // If not connected to a server when triggered
    // Then open myDialog
    if ((is_connected == false))
    {
        update_Timer.stop();
        MyDialog mDialog;
        mDialog.setModal(true);

        mDialog.exec();

        MyName=mDialog.getName();
        HostIP = mDialog.getIP();
        HostPort=mDialog.getPort().toInt();
        m_MyProfession=mDialog.getProfession();
        uiConfig->professionComboBox->setCurrentIndex(m_MyProfession);
        Initialize();
    }
    // Otherwise stop the timer and abort the connection
    else
    {
        update_Timer.stop();
        socket->abort();
        ui->progressBar_0->setVisible(false);
        ui->progressBar_1->setVisible(false);
        ui->progressBar_2->setVisible(false);
        ui->progressBar_3->setVisible(false);
        ui->progressBar_4->setVisible(false);
        ui->progressBar_5->setVisible(false);
        ui->progressBar_6->setVisible(false);
        ui->progressBar_7->setVisible(false);
        ui->progressBar_8->setVisible(false);
        ui->progressBar_9->setVisible(false);
        is_connected = false;
        HostIP="";

        //Go back to the initializer
        Initialize();
    }
}

void GW2::MainWindow::on_actionClose_triggered()
{
    // Let's abort the connection to the server before closing
    if ((is_connected == true)) socket->abort();
}

void GW2::MainWindow::on_pushButton_clicked(){
    QString downloadlink = "http://gw2specs.com/download";
    QDesktopServices::openUrl(QUrl(downloadlink));
}

void GW2::MainWindow::on_pushButton_2_clicked(){
    QString changeloglink = "http://gw2specs.com/changelog";
    QDesktopServices::openUrl(QUrl(changeloglink));
}

void MainWindow::updateCombatCourse()
{
    if(countCombat > 0){
        //Filling Up Variable For saving later into the Log
        combatCourse += m_Time + " | " + QString::number(m_Dps) + " | " + QString::number(m_realDps) + " | " + QString::number(m_Dmg) + "\r\n";
    }
}

void MainWindow::writeFile(QString separator)
{
    QString tableSep = "";
    QString fileEnding = "txt";
    if(separator == ";"){
        //If we're in CSV Mode
        tableSep = separator;
        fileEnding = QString::fromLatin1("csv");
    }else{
        //If we're in TXT Mode
        tableSep = "|";
        fileEnding = QString::fromLatin1("txt");
    }

    QDir dir("combatLog/");
    if (!dir.exists()) {dir.mkpath(".");}

    unsigned long c;
    double c1,c2,c3,c4;
    c2=m_condiDmg;
    c3=m_Dps;
    c4=m_Dmg;
    c1=c2*c3;
    if (m_Dmg>0)c=round(c1/c4);else c=0;

    const QDateTime now = QDateTime::currentDateTime();
    const QString timestamp = now.toString(QLatin1String("yyyy-MM-dd_hh''mm''ss"));
    const QString filename = QString::fromLatin1("combatLog/combatLog-%1.%2").arg(timestamp).arg(fileEnding);

    QFile file( filename );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        if(separator == ": "){
            stream << "*************************************\r\n";
            stream << "*         SPECS Log - v 1.0         *\r\n";
            stream << "*************************************\r\n";
            stream << "\r\n";
        }
        if(separator == ";"){
            stream << "Personal Data\r\n";
        }

        stream << "Time" << separator << m_Time << "\r\n";
        stream << "Hitcounter" << separator << hitCounter << "\r\n";
        stream << "Highest Hit" << separator << m_MaxDmg << "\r\n";
        stream << "CritChance" << separator << m_critChance << "%" << "\r\n";
        stream << "\r\n";
        stream << "DPS" << separator << m_Dps << "\r\n";
        stream << "DMG" << separator << m_Dmg << "\r\n";
        stream << "\r\n";
        stream << "CondiDPS" << separator << c << "\r\n";
        stream << "CondiDMG" << separator << m_condiDmg << "\r\n";
        stream << "\r\n\r\n";

        if(is_connected == true){
            if(separator == ": "){
                stream << "*************************************\r\n";
                stream << "*            Group Info             *\r\n";
                stream << "*************************************\r\n";
            }
            stream << "\r\n";
            stream << "AvgDPS" << separator << AvgDPS << "\r\n";
            stream << "GroupDPS" << separator << GrpDPS << "\r\n";
            stream << "GroupDMG " << separator << GrpDmg << "\r\n";
            stream << "\r\n\r\n\r\n";
            QStringList profList;
            profList << "None" << "Elementalist" << "Engineer" << "Guardian" << "Mesmer" << "Necromancer" << "Ranger" << "Revenant" << "Thief" << "Warrior";
            stream <<  "Name       " << tableSep << "   DPS  " << tableSep << "    DMG    " << tableSep << " Class\r\n";
            for(int i=0;i<10;i++){
                if(PosName[i][0] !=0){
                    QString tempName = PosName[i];
                    QString tempDPS = QString::number(PosDPS[i]);
                    QString tempDMG = QString::number(PosDmg[i]);
                    QString modDMG;

                    if (tempDMG.size() > 3) {
                        modDMG = tempDMG.rightJustified(7, ' ');
                    } if (tempDMG.size() > 6) {
                        modDMG = tempDMG.rightJustified(7, ' ');
                    } else {
                        modDMG = tempDMG.rightJustified(8, ' ');
                    }

                    if (tempDMG.size() > 3) {
                    modDMG.insert(modDMG.size() - 3, ".");
                    }
                    if (tempDMG.size() > 6) {
                    modDMG.insert(modDMG.size() - 7, ".");
                    }


                    if(tempDPS.size() > 3){tempDPS.insert(tempDPS.size() -3, ".");}
                    stream << tempName.leftJustified(10, ' ') << " " << tableSep << " " << tempDPS.rightJustified(6, ' ') << " " << tableSep << " " << modDMG << " " << tableSep << " " << profList[PosProf[i]] << "\r\n";
                }

            }
            stream << "\r\n\r\n\r\n";
        }
        stream << " Time(hh:mm:ss) " << tableSep << "  AvgDPS  " << tableSep << "  RealDPS  " << tableSep << "  DMG\r\n";
        if(separator == ";"){
            combatCourse.replace(QString("|"),QString(";"));
        }
        combatCourse.replace(QString("m "),QString(":"));
        combatCourse.replace(QString("s"),QString(""));
        stream << combatCourse;
    }
}

void GW2::MainWindow::on_actionActionSave_triggered()
{
    //Show Popup here to choose between TXT and CSV
        QDialog *savePopup= new QDialog();
        QVBoxLayout *layout = new QVBoxLayout(savePopup);
        QPushButton *txtButton = new QPushButton(this);
        QPushButton *csvButton = new QPushButton(this);
        QLabel *chooseLabel = new QLabel(this);

        layout->addWidget(chooseLabel);
        layout->addWidget(txtButton);
        layout->addWidget(csvButton);

        layout->setMargin(10);
        chooseLabel->setText("Choose the File Format you like!");
        txtButton->setText("*.txt");
        csvButton->setText("*.csv");

        savePopup->setWindowTitle("Gw2SPECS Save v1.0");
        savePopup->setStyleSheet("background:#f2f2f2;");
        savePopup->setWindowFlags(Qt::WindowStaysOnTopHint| Qt::WindowCloseButtonHint);
        savePopup->show();

        //need to create 2 Buttons
        //set dialog settings to always on top frameless
        //connect Buttons to functions
        QObject::connect(txtButton, SIGNAL(clicked(bool)), this, SLOT(writeTxt()));
        QObject::connect(txtButton, SIGNAL(clicked(bool)), savePopup, SLOT(hide()));
        QObject::connect(csvButton, SIGNAL(clicked(bool)), this, SLOT(writeCsv()));
        QObject::connect(csvButton, SIGNAL(clicked(bool)), savePopup, SLOT(hide()));
}

void GW2::MainWindow::writeTxt()
{
    QString txtSep = ": ";
    writeFile(txtSep);
}

void GW2::MainWindow::writeCsv()
{
    QString csvSep = ";";
    writeFile(csvSep);
}

void GW2::MainWindow::ShowContextMenu(const QPoint& pos)
{
    QPoint globalPos = ui->scrollArea->viewport()->mapToGlobal(pos);
    myMenu.exec(globalPos);
}

void GW2::MainWindow::ShowContextMenuDetails(const QPoint& pos)
{
    QPoint globalPos = ui->widget->mapToGlobal(pos);
    myMenu.exec(globalPos);
}

void GW2::MainWindow::ShowContextMenuGraph(const QPoint& pos)
{
    QPoint globalPos = ui->widget_4->mapToGlobal(pos);
    myMenu.exec(globalPos);
}

bool GW2::MainWindow::connectToServ(bool toggled){
    if (toggled)
    {
        connectServer->setIcon(QIcon(":/connected"));
        connectServer->setText("Disconnect");
        toggled = true;
    }
    else
    {
        connectServer->setIcon(QIcon(":/connect"));
        connectServer->setText("Connect");
        toggled = false;
    }
    return toggled;
}

bool GW2::MainWindow::resetAutomatic(bool toggled){
    if (toggled)
    {
        autoReset->setIcon(QIcon(":/Auto_Reset_Active"));
        autoReset->setText("Auto Reset Off");
        toggled = true;
    }
    else
    {
        autoReset->setIcon(QIcon(":/Auto_Reset"));
        autoReset->setText("Auto Reset On");
        toggled = false;
    }
    return toggled;
}


void GW2::MainWindow::action_fixOnTop(){
    this->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->activateWindow();
    this->setFocus();
    this->show();
}

void GW2::MainWindow::action_combatMode(){
    //Make click-through
    HWND winHandle  = (HWND)winId();
    ShowWindow(winHandle, SW_HIDE);
    SetWindowLong(winHandle, GWL_EXSTYLE, GetWindowLong(winHandle, GWL_EXSTYLE)| WS_EX_APPWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT);
    ShowWindow(winHandle, SW_SHOW);
    action_fixOnTop();
    qDebug()<< "CombatMode Activated";

    QVBoxLayout *layout = new QVBoxLayout(combatDialog);
    QLabel *label = new QLabel(this);
    resetCombatMode->setText("Reset Combatmode!");
    label->setText("Welcome to Gw2SPECS CombatMode\nPlease minimize this window so it cannot interfere anymore.\n Gw2SPECS is no longer clickable and all clicks fall through on the window behind\nIf you want to end CombatMode simply click the Button below!\nEnjoy.");
    layout->addWidget(label);
    layout->addWidget(resetCombatMode);
    layout->setMargin(25);
    combatDialog->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::CustomizeWindowHint);
    combatDialog->setStyleSheet("QDialog{background-color: rgb(32, 43, 47);}QLabel{color:#f2f2f2;text-align:center;}QPushButton{background-color: rgb(52, 63, 67);color:#f2f2f2;text-align:center;}");
    combatDialog->show();
    resetCombatMode->show();
}

void GW2::MainWindow::action_resetCombatMode(){
    HWND winHandle  = (HWND)winId();
    ShowWindow(winHandle, SW_HIDE);
    SetWindowLong(winHandle, GWL_EXSTYLE, GetWindowLong(winHandle, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    ShowWindow(winHandle, SW_SHOW);
    action_fixOnTop();
    combatDialog->close();
    qDebug()<<"CombatMode Deactivated";
}


void GW2::MainWindow::runMe(){
    ui->widget_4->addLayer("abovemain", ui->widget_4->layer("main"), QCustomPlot::limAbove);

    ui->widget_4->addGraph(); // green line
    ui->widget_4->graph(0)->setPen(QPen(QColor(46, 204, 113)));
    ui->widget_4->graph(0)->setLayer("abovemain");

    ui->widget_4->addGraph(); // purple line
    ui->widget_4->graph(1)->setPen(QPen(QColor(142, 68, 173)));
    ui->widget_4->graph(1)->setLayer("abovemain");

    ui->widget_4->addGraph(); // red line
    ui->widget_4->graph(2)->setPen(QPen(QColor(231, 76, 60, 125)));
    ui->widget_4->graph(2)->setBrush(QBrush(QColor(231, 76, 60, 125)));
    ui->widget_4->graph(2)->setLayer("main");

    ui->widget_4->addGraph(); // blue line
    ui->widget_4->graph(3)->setPen(QPen(QColor(41,128, 185)));
    ui->widget_4->graph(3)->setLayer("abovemain");

    ui->widget_4->addGraph();//yellow
    ui->widget_4->graph(4)->setPen(QPen(QColor(243,156,18)));
    ui->widget_4->graph(4)->setLayer("abovemain");


    ui->widget_4->xAxis->setAutoTickStep(false);
    ui->widget_4->xAxis->setTickStep(1);
    ui->widget_4->yAxis->setLabel("DPS");
    //ui->widget_4->rescaleAxes();
    //ui->widget_4->axisRect()->setupFullAxesBox();
    ui->widget_4->yAxis->setRange(0,7000);
    ui->widget_4->yAxis2->setRange(0,7000);
    ui->widget_4->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    ui->widget_4->xAxis->setBasePen(QPen(Qt::white, 1));
    ui->widget_4->yAxis->setBasePen(QPen(Qt::white, 1));
    ui->widget_4->xAxis->setTickPen(QPen(Qt::white, 1));
    ui->widget_4->yAxis->setTickPen(QPen(Qt::white, 1));
    ui->widget_4->xAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->widget_4->yAxis->setSubTickPen(QPen(Qt::white, 1));
    ui->widget_4->xAxis->setTickLabelColor(Qt::white);
    ui->widget_4->yAxis->setTickLabelColor(Qt::white);
    ui->widget_4->yAxis->setLabelColor(Qt::white);
    ui->widget_4->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    ui->widget_4->yAxis->grid()->setZeroLinePen(Qt::NoPen);


    //Graph Background Color
    ui->widget_4->setBackground(Qt::transparent);

    //Graph Axis Color
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    ui->widget_4->axisRect()->setBackground(Qt::transparent);


    // make left and bottom axes transfer their ranges to right and top axes:
    connect(ui->widget_4->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->widget_4->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->widget_4->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->widget_4->yAxis2, SLOT(setRange(QCPRange)));

    ui->widget_4->replot();
}
static double lastPointKey = 0;

void GW2::MainWindow::realTimeDataSlot(int dps, int cdps,int avgdps, int msecs, int five_sdps, int realDps){
    // calculate two new data points:
    double key = msecs/1000;

    if (key-lastPointKey > 0.01) // at most add point every 10 ms
    {
      double value0 = dps;
      double value1 = cdps;
      double value2 = avgdps;
      double value3 = five_sdps;
      double value4 = realDps;

      // add data to lines:
      ui->widget_4->graph(0)->addData(key, value0);
      ui->widget_4->graph(1)->addData(key, value1);
      ui->widget_4->graph(2)->addData(key, value3);
      ui->widget_4->graph(3)->addData(key, value4);

      if((is_connected == true)){
          ui->widget_4->graph(4)->addData(key - 1, value2);
      }
      // rescale value (vertical) axis to fit the current data:
      ui->widget_4->graph(0)->rescaleValueAxis(true);
      ui->widget_4->graph(1)->rescaleValueAxis(true);
      ui->widget_4->graph(2)->rescaleValueAxis(true);
      ui->widget_4->graph(3)->rescaleValueAxis(true);

      ui->widget_4->graph(0)->rescaleKeyAxis(true);
      ui->widget_4->graph(1)->rescaleKeyAxis(true);
      ui->widget_4->graph(2)->rescaleKeyAxis(true);
      ui->widget_4->graph(3)->rescaleKeyAxis(true);

      if(is_connected == true){
          ui->widget_4->graph(4)->rescaleValueAxis(true);
          ui->widget_4->graph(4)->rescaleKeyAxis(true);
      }
      lastPointKey = key;
    }
    // make key axis range scroll with the data (at a constant range size of 8):
    ui->widget_4->xAxis->setRange(key+0.25, key, Qt::AlignRight);
    ui->widget_4->xAxis->setTickStep(round(key/5));
    ui->widget_4->replot();
}

void GW2::MainWindow::resetGraph(){
    lastPointKey = 0;
    for(int i=0;i<5;i++){
        ui->widget_4->graph(i)->clearData();
        ui->widget_4->graph(i)->addData(0,0);
    }
    ui->widget_4->yAxis->setRange(0,7000);
    ui->widget_4->yAxis2->setRange(0,7000);
    ui->widget_4->replot();
}

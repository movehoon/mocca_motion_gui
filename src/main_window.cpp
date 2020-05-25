/**
 * @file /src/main_window.cpp
 *
 * @brief Implementation for the qt gui.
 *
 * @date February 2011
 **/
/*****************************************************************************
** Includes
*****************************************************************************/

#include <QtGui>
#include <QMessageBox>
#include <iostream>
#include <fstream>
#include <string>
#include "../include/mocca_motion_gui/main_window.hpp"
// #include <filesystem>
#include <qtextcodec.h>
#include <dirent.h>
#include <qstring.h>

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace mocca_motion_gui {

using namespace Qt;
using namespace std;
// namespace fs = std::filesystem;

/*****************************************************************************
** Implementation [MainWindow]
*****************************************************************************/

const char * MOTION_DIR = "/home/parallels/JsonData";

vector<string> list_dir(const char *path) {
	vector<string> files;

	struct dirent *entry;
	DIR *dir = opendir(path);

	cout << "list_dir of " << path << endl;
	if (dir == NULL) {
		return files;
	}
	while ((entry = readdir(dir)) != NULL) {
		string file_name = entry->d_name;
		if(file_name.substr(file_name.find_last_of(".") + 1) == "json") {
			files.push_back(entry->d_name);
			// cout << entry->d_name << endl;
		}
	}
	closedir(dir);
	return files;
}


MainWindow::MainWindow(int argc, char** argv, QWidget *parent)
	: QMainWindow(parent)
	, qnode(argc,argv)
{
	ui.setupUi(this); // Calling this incidentally connects all ui's triggers to on_...() callbacks in this class.
    QObject::connect(ui.actionAbout_Qt, SIGNAL(triggered(bool)), qApp, SLOT(aboutQt())); // qApp is a global variable for the application

    ReadSettings();
	setWindowIcon(QIcon(":/images/icon.png"));
	ui.tab_manager->setCurrentIndex(0); // ensure the first tab is showing - qt-designer should have this already hardwired, but often loses it (settings?).
    QObject::connect(&qnode, SIGNAL(rosShutdown()), this, SLOT(close()));

	/*********************
	** Logging
	**********************/
	ui.view_logging->setModel(qnode.loggingModel());
    QObject::connect(&qnode, SIGNAL(loggingUpdated()), this, SLOT(updateLoggingView()));

    /*********************
    ** Auto Start
    **********************/
    if ( ui.checkbox_remember_settings->isChecked() ) {
        on_button_connect_clicked(true);
    }

    vector<string> files = list_dir(MOTION_DIR);
    if (files.size() > 0) {
    	for (vector<string>::iterator iter = files.begin(); iter < files.end(); iter++) {
    		string str = *iter;
		    ui.comboBox_motion_list->addItem(QString::fromLocal8Bit(str.c_str()));
    	}
    }
}

MainWindow::~MainWindow() {}

/*****************************************************************************
** Implementation [Slots]
*****************************************************************************/

void MainWindow::showNoMasterMessage() {
	QMessageBox msgBox;
	msgBox.setText("Couldn't find the ros master.");
	msgBox.exec();
    close();
}

/*
 * These triggers whenever the button is clicked, regardless of whether it
 * is already checked or not.
 */

void MainWindow::on_button_connect_clicked(bool check ) {
	if ( ui.checkbox_use_environment->isChecked() ) {
		if ( !qnode.init() ) {
			showNoMasterMessage();
		} else {
			ui.button_connect->setEnabled(false);
		}
	} else {
		if ( ! qnode.init(ui.line_edit_master->text().toStdString(),
				   ui.line_edit_host->text().toStdString()) ) {
			showNoMasterMessage();
		} else {
			ui.button_connect->setEnabled(false);
			ui.line_edit_master->setReadOnly(true);
			ui.line_edit_host->setReadOnly(true);
			ui.line_edit_topic->setReadOnly(true);
		}
	}
}


void MainWindow::on_checkbox_use_environment_stateChanged(int state) {
	bool enabled;
	if ( state == 0 ) {
		enabled = true;
	} else {
		enabled = false;
	}
	ui.line_edit_master->setEnabled(enabled);
	ui.line_edit_host->setEnabled(enabled);
	//ui.line_edit_topic->setEnabled(enabled);
}

void MainWindow::on_checkBox_torque_enable_stateChanged(int state) {
	if (state > 0)
		state = 1;
	qnode.torque(state);
}

void MainWindow::on_checkBox_play_from_motion_editor_stateChanged(int state) {
	QMessageBox msgBox;
	msgBox.setText(to_string(state).c_str());
	msgBox.exec();
}

void MainWindow::on_checkBox_play_physical_mocca_robot_stateChanged(int state) {

}

void MainWindow::on_button_play_motion_clicked(bool check) {
	QString selectedComboString = ui.comboBox_motion_list->currentText().toLocal8Bit();
	cout << "Select: " << selectedComboString.toStdString().c_str() << endl; 

	// QString localeStr = codec->toUnicode(ui.comboBox_motion_list->currentText().toStdString());
	string selectedFilename = selectedComboString.toStdString();
	cout << "Play: " << selectedFilename.c_str() << endl; 

	string filepath = MOTION_DIR;
	filepath += "/";
	filepath += selectedFilename;
	cout << "Open: " << filepath << endl;

	string line;
	string fliedata;
	ifstream ifs (filepath);
	if (ifs.is_open())
	{
		while ( getline (ifs,line) )
		{
			fliedata += line;
			cout << line << '\n';
		}
		ifs.close();

		qnode.playMotion(fliedata);
	}
}


/*****************************************************************************
** Implemenation [Slots][manually connected]
*****************************************************************************/

/**
 * This function is signalled by the underlying model. When the model changes,
 * this will drop the cursor down to the last line in the QListview to ensure
 * the user can always see the latest log message.
 */
void MainWindow::updateLoggingView() {
        ui.view_logging->scrollToBottom();
}

/*****************************************************************************
** Implementation [Menu]
*****************************************************************************/

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this, tr("About ..."),tr("<h2>PACKAGE_NAME Test Program 0.10</h2><p>Copyright Yujin Robot</p><p>This package needs an about description.</p>"));
}

/*****************************************************************************
** Implementation [Configuration]
*****************************************************************************/

void MainWindow::ReadSettings() {
    QSettings settings("Qt-Ros Package", "mocca_motion_gui");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    QString master_url = settings.value("master_url",QString("http://192.168.1.2:11311/")).toString();
    QString host_url = settings.value("host_url", QString("192.168.1.3")).toString();
    //QString topic_name = settings.value("topic_name", QString("/chatter")).toString();
    ui.line_edit_master->setText(master_url);
    ui.line_edit_host->setText(host_url);
    //ui.line_edit_topic->setText(topic_name);
    bool remember = settings.value("remember_settings", false).toBool();
    ui.checkbox_remember_settings->setChecked(remember);
    bool checked = settings.value("use_environment_variables", false).toBool();
    ui.checkbox_use_environment->setChecked(checked);
    if ( checked ) {
    	ui.line_edit_master->setEnabled(false);
    	ui.line_edit_host->setEnabled(false);
    	//ui.line_edit_topic->setEnabled(false);
    }
}

void MainWindow::WriteSettings() {
    QSettings settings("Qt-Ros Package", "mocca_motion_gui");
    settings.setValue("master_url",ui.line_edit_master->text());
    settings.setValue("host_url",ui.line_edit_host->text());
    //settings.setValue("topic_name",ui.line_edit_topic->text());
    settings.setValue("use_environment_variables",QVariant(ui.checkbox_use_environment->isChecked()));
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("remember_settings",QVariant(ui.checkbox_remember_settings->isChecked()));

}

void MainWindow::closeEvent(QCloseEvent *event)
{
	WriteSettings();
	QMainWindow::closeEvent(event);
}

}  // namespace mocca_motion_gui


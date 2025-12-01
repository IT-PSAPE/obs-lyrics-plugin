#pragma once

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QDockWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QPointer>

class LyricsControl : public QDockWidget {
	Q_OBJECT

public:
	LyricsControl(QWidget *parent = nullptr);
	~LyricsControl();

private slots:
	void OnSourceSelected(OBSWeakSource source);
	void OnSongChanged(int index);
	void OnPrevClicked();
	void OnNextClicked();
	void OnToggleClicked();

private:
	void UpdateUI();
	void RefreshSongs();
	void UpdateSourceSettings();

	QPointer<QWidget> container;
	QComboBox *songCombo;
	QPushButton *prevBtn;
	QPushButton *nextBtn;
	QPushButton *toggleBtn;
	QLabel *statusLabel;

	OBSWeakSource weakSource;
	QString currentFolder;
	QStringList songFiles;
	
	/* State tracking to avoid loops */
	bool ignoreSignals;
};

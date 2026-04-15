#include "stdafx.h"
#include "OptionsWindow.h"
#include "SandMan.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/Common.h"

void COptionsWindow::CreateAutoStart()
{
	connect(ui.chkAutoStartEnabled, SIGNAL(clicked(bool)), this, SLOT(OnAutoStartEnabledChanged()));
	connect(ui.btnAddAutoStartEntry, SIGNAL(clicked(bool)), this, SLOT(OnAddAutoStartEntry()));
	connect(ui.btnBrowseAutoStartExe, SIGNAL(clicked(bool)), this, SLOT(OnBrowseAutoStartExe()));
	connect(ui.btnAutoStartFromStartMenu, SIGNAL(clicked(bool)), this, SLOT(OnAutoStartFromStartMenu()));
	connect(ui.btnAutoStartMoveUp, SIGNAL(clicked(bool)), this, SLOT(OnAutoStartMoveUp()));
	connect(ui.btnAutoStartMoveDown, SIGNAL(clicked(bool)), this, SLOT(OnAutoStartMoveDown()));
	connect(ui.btnDelAutoStartEntry, SIGNAL(clicked(bool)), this, SLOT(OnDelAutoStartEntry()));
	connect(ui.spinAutoStartDelay, SIGNAL(valueChanged(int)), this, SLOT(OnAutoStartDelayChanged()));
	connect(ui.treeAutoStart, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnAutoStartEnabledChanged()));
}

void COptionsWindow::LoadAutoStart()
{
	m_HoldChange = true;

	ui.chkAutoStartEnabled->setChecked(m_pBox->GetBool("AutoStartOnLogonEnabled", false));
	ui.spinAutoStartDelay->setValue(m_pBox->GetNum("AutoStartOnLogonDelay", 5));

	ui.treeAutoStart->clear();

	auto addEntry = [this](const QString& Value, bool disabled) {
		QStringList Parts = Value.split("|");
		QString Command = Parts.value(0);
		QString WorkDir = Parts.value(1);
		QString DelayStr = Parts.value(2);
		int Delay = DelayStr.isEmpty() ? 0 : DelayStr.toInt();

		QString ProgramName;
		QString CmdOnly = Command;
		if (CmdOnly.startsWith("\"")) {
			int end = CmdOnly.indexOf("\"", 1);
			if (end > 0) ProgramName = CmdOnly.mid(1, end - 1);
		}
		if (ProgramName.isEmpty()) ProgramName = CmdOnly.section(' ', 0, 0);
		ProgramName = ProgramName.mid(ProgramName.lastIndexOf('\\') + 1);
		ProgramName = ProgramName.mid(ProgramName.lastIndexOf('/') + 1);

		QTreeWidgetItem* pItem = new QTreeWidgetItem();
		pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
		pItem->setCheckState(0, disabled ? Qt::Unchecked : Qt::Checked);
		pItem->setText(0, ProgramName);
		pItem->setData(0, Qt::UserRole, Command);
		pItem->setText(1, Command);
		pItem->setText(2, WorkDir);
		pItem->setText(3, QString::number(Delay));
		pItem->setData(3, Qt::UserRole, Delay);
		ui.treeAutoStart->addTopLevelItem(pItem);
	};

	foreach(const QString & Value, m_pBox->GetTextList("AutoStartOnLogon", m_Template))
		addEntry(Value, false);
	foreach(const QString & Value, m_pBox->GetTextList("AutoStartOnLogonDisabled", m_Template))
		addEntry(Value, true);

	m_AutoStartChanged = false;
	m_HoldChange = false;
}

void COptionsWindow::SaveAutoStart()
{
	WriteAdvancedCheck(ui.chkAutoStartEnabled, "AutoStartOnLogonEnabled", "y", "");

	int delay = ui.spinAutoStartDelay->value();
	if (delay != 5)
		WriteText("AutoStartOnLogonDelay", QString::number(delay));
	else
		m_pBox->DelValue("AutoStartOnLogonDelay");

	QStringList Entries;
	QStringList EntriesDisabled;

	for (int i = 0; i < ui.treeAutoStart->topLevelItemCount(); i++) {
		QTreeWidgetItem* pItem = ui.treeAutoStart->topLevelItem(i);
		QString Command = pItem->data(0, Qt::UserRole).toString();
		if (Command.isEmpty()) Command = pItem->text(1);
		QString WorkDir = pItem->text(2);
		int Delay = pItem->data(3, Qt::UserRole).toInt();

		QString Entry = Command;
		if (!WorkDir.isEmpty() || Delay > 0)
			Entry += "|" + WorkDir;
		if (Delay > 0)
			Entry += "|" + QString::number(Delay);

		if (pItem->checkState(0) == Qt::Checked)
			Entries.append(Entry);
		else
			EntriesDisabled.append(Entry);
	}

	WriteTextList("AutoStartOnLogon", Entries);
	WriteTextList("AutoStartOnLogonDisabled", EntriesDisabled);

	m_AutoStartChanged = false;
}

void COptionsWindow::OnAddAutoStartEntry()
{
	QString Value = QInputDialog::getText(this, "Sandboxie-Plus", 
		tr("Please enter the command line for the program to auto-start in this sandbox on user logon"), QLineEdit::Normal);
	if (Value.isEmpty())
		return;

	QString ProgramName = Value;
	if (ProgramName.startsWith("\"")) {
		int end = ProgramName.indexOf("\"", 1);
		if (end > 0) ProgramName = ProgramName.mid(1, end - 1);
	} else {
		ProgramName = ProgramName.section(' ', 0, 0);
	}
	ProgramName = ProgramName.mid(ProgramName.lastIndexOf('\\') + 1);
	ProgramName = ProgramName.mid(ProgramName.lastIndexOf('/') + 1);

	QTreeWidgetItem* pItem = new QTreeWidgetItem();
	pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
	pItem->setCheckState(0, Qt::Checked);
	pItem->setText(0, ProgramName);
	pItem->setData(0, Qt::UserRole, Value);
	pItem->setText(1, Value);
	pItem->setText(2, "");
	pItem->setText(3, "0");
	pItem->setData(3, Qt::UserRole, 0);
	ui.treeAutoStart->addTopLevelItem(pItem);

	m_AutoStartChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnBrowseAutoStartExe()
{
	auto pBoxPlus = m_pBox.objectCast<CSandBoxPlus>();
	QString Root;
	if (pBoxPlus) Root = pBoxPlus->GetFileRoot();
	if (Root.isEmpty()) Root = "C:\\";

	QString Path = QFileDialog::getOpenFileName(this, tr("Select Program"), Root,
		tr("Executables (*.exe *.bat *.cmd);;All files (*)")).replace("/", "\\");
	if (Path.isEmpty())
		return;

	if (pBoxPlus) {
		QString BoxRoot = pBoxPlus->GetFileRoot();
		if (!BoxRoot.isEmpty() && Path.startsWith(BoxRoot, Qt::CaseInsensitive))
			Path = pBoxPlus->MakeBoxCommand(Path);
	}

	QString ProgramName = Path.mid(Path.lastIndexOf('\\') + 1);

	QTreeWidgetItem* pItem = new QTreeWidgetItem();
	pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
	pItem->setCheckState(0, Qt::Checked);
	pItem->setText(0, ProgramName);
	pItem->setData(0, Qt::UserRole, "\"" + Path + "\"");
	pItem->setText(1, "\"" + Path + "\"");
	pItem->setText(2, "");
	pItem->setText(3, "0");
	pItem->setData(3, Qt::UserRole, 0);
	ui.treeAutoStart->addTopLevelItem(pItem);

	m_AutoStartChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnAutoStartFromStartMenu()
{
	auto pBoxPlus = m_pBox.objectCast<CSandBoxPlus>();
	if (!pBoxPlus)
		return;

	QList<CSandBoxPlus::SLink> StartMenu = pBoxPlus->GetStartMenu();
	if (StartMenu.isEmpty()) {
		QMessageBox::information(this, "Sandboxie-Plus", tr("No start menu entries found in this sandbox. Run and install programs in the sandbox first."));
		return;
	}

	QStringList Items;
	for (const auto& Link : StartMenu)
		Items.append(Link.Name + "  [" + Link.Target + "]");

	bool ok;
	QString Selected = QInputDialog::getItem(this, "Sandboxie-Plus",
		tr("Select a program from the sandbox start menu:"), Items, 0, false, &ok);
	if (!ok || Selected.isEmpty())
		return;

	int idx = Items.indexOf(Selected);
	if (idx < 0 || idx >= StartMenu.count())
		return;

	const CSandBoxPlus::SLink& Link = StartMenu[idx];

	QString Command = Link.Target;
	if (!Link.Arguments.isEmpty())
		Command += " " + Link.Arguments;
	
	if (pBoxPlus)
		Command = pBoxPlus->MakeBoxCommand(Command);

	QTreeWidgetItem* pItem = new QTreeWidgetItem();
	pItem->setFlags(pItem->flags() | Qt::ItemIsUserCheckable);
	pItem->setCheckState(0, Qt::Checked);
	pItem->setText(0, Link.Name);
	pItem->setData(0, Qt::UserRole, Command);
	pItem->setText(1, Command);
	pItem->setText(2, Link.WorkDir);
	pItem->setText(3, "0");
	pItem->setData(3, Qt::UserRole, 0);
	ui.treeAutoStart->addTopLevelItem(pItem);

	m_AutoStartChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnAutoStartMoveUp()
{
	int index = ui.treeAutoStart->indexOfTopLevelItem(ui.treeAutoStart->currentItem());
	if (index <= 0)
		return;
	QTreeWidgetItem* pItem = ui.treeAutoStart->takeTopLevelItem(index);
	ui.treeAutoStart->insertTopLevelItem(index - 1, pItem);
	ui.treeAutoStart->setCurrentItem(pItem);

	m_AutoStartChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnAutoStartMoveDown()
{
	int index = ui.treeAutoStart->indexOfTopLevelItem(ui.treeAutoStart->currentItem());
	if (index < 0 || index >= ui.treeAutoStart->topLevelItemCount() - 1)
		return;
	QTreeWidgetItem* pItem = ui.treeAutoStart->takeTopLevelItem(index);
	ui.treeAutoStart->insertTopLevelItem(index + 1, pItem);
	ui.treeAutoStart->setCurrentItem(pItem);

	m_AutoStartChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnDelAutoStartEntry()
{
	QTreeWidgetItem* pItem = ui.treeAutoStart->currentItem();
	if (!pItem)
		return;
	delete pItem;

	m_AutoStartChanged = true;
	OnOptChanged();
}

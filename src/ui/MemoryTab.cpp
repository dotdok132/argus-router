#include "MemoryTab.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QDir>

MemoryTab::MemoryTab(MemoryManager *memMgr, QWidget *parent)
    : QWidget(parent), m_memMgr(memMgr) {
    setupUi();
    if (m_memMgr) {
        connect(m_memMgr, &MemoryManager::memoryListUpdated, this, &MemoryTab::refreshMemoryList);
        refreshMemoryList();
    }
}

void MemoryTab::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel - Memory Files List
    QWidget *leftContainer = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    QHBoxLayout *leftToolbar = new QHBoxLayout();
    QPushButton *btnNew = new QPushButton("+ New Memory");
    btnNew->setObjectName("PrimaryButton");
    connect(btnNew, &QPushButton::clicked, this, &MemoryTab::onCreateClicked);

    QPushButton *btnDelete = new QPushButton("Delete");
    btnDelete->setObjectName("TableGhostDanger");
    connect(btnDelete, &QPushButton::clicked, this, &MemoryTab::onDeleteClicked);

    leftToolbar->addWidget(btnNew);
    leftToolbar->addWidget(btnDelete);

    m_listWidget = new QListWidget();
    m_listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #252526;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            color: #d4d4d4;
            font-size: 12px;
        }
        QListWidget::item {
            padding: 8px 10px;
            border-bottom: 1px solid #2d2d2d;
        }
        QListWidget::item:selected {
            background-color: #04395e;
            color: #ffffff;
            font-weight: 600;
        }
        QListWidget::item:hover:!selected {
            background-color: #2a2d2e;
        }
    )");

    connect(m_listWidget, &QListWidget::currentItemChanged, this, &MemoryTab::onMemorySelected);

    leftLayout->addLayout(leftToolbar);
    leftLayout->addWidget(m_listWidget);

    // Right Panel - Markdown Editor
    QWidget *rightContainer = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(8);

    QHBoxLayout *rightToolbar = new QHBoxLayout();
    m_lblCurrentFile = new QLabel("Select a memory file to edit");
    m_lblCurrentFile->setStyleSheet("color: #858585; font-size: 12px; font-weight: 600; font-family: monospace;");

    m_btnSave = new QPushButton("Save Changes");
    m_btnSave->setObjectName("PrimaryButton");
    m_btnSave->setEnabled(false);
    connect(m_btnSave, &QPushButton::clicked, this, &MemoryTab::onSaveClicked);

    rightToolbar->addWidget(m_lblCurrentFile);
    rightToolbar->addStretch();
    rightToolbar->addWidget(m_btnSave);

    m_textEditor = new QTextEdit();
    m_textEditor->setStyleSheet(R"(
        QTextEdit {
            background-color: #1e1e1e;
            border: 1px solid #3c3c3c;
            border-radius: 4px;
            color: #d4d4d4;
            font-family: monospace;
            font-size: 13px;
            line-height: 1.4;
            padding: 8px;
        }
    )");

    rightLayout->addLayout(rightToolbar);
    rightLayout->addWidget(m_textEditor);

    splitter->addWidget(leftContainer);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(splitter);
}

void MemoryTab::refreshMemoryList() {
    if (!m_memMgr || !m_listWidget) return;

    QString currentSel = m_activeFileName;
    m_listWidget->clear();

    QStringList files = m_memMgr->listMemories();
    int selIndex = -1;

    for (int i = 0; i < files.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(files[i]);
        m_listWidget->addItem(item);
        if (files[i] == currentSel) {
            selIndex = i;
        }
    }

    if (selIndex >= 0) {
        m_listWidget->setCurrentRow(selIndex);
    } else if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void MemoryTab::onMemorySelected(QListWidgetItem *current, QListWidgetItem *previous) {
    Q_UNUSED(previous);
    if (!current || !m_memMgr) {
        m_activeFileName.clear();
        m_lblCurrentFile->setText("No memory selected");
        m_textEditor->clear();
        m_btnSave->setEnabled(false);
        return;
    }

    m_activeFileName = current->text();
    m_lblCurrentFile->setText(QString("Editing: .memory/%1").arg(m_activeFileName));
    QString content = m_memMgr->readMemory(m_activeFileName);
    m_textEditor->setPlainText(content);
    m_btnSave->setEnabled(true);
}

void MemoryTab::onSaveClicked() {
    if (m_activeFileName.isEmpty() || !m_memMgr) return;

    QString newContent = m_textEditor->toPlainText();
    bool ok = m_memMgr->saveMemory(m_activeFileName, newContent);
    if (ok) {
        m_lblCurrentFile->setText(QString("Editing: .memory/%1 [SAVED]").arg(m_activeFileName));
    } else {
        QMessageBox::warning(this, "Save Failed", QString("Failed to save memory file: %1").arg(m_activeFileName));
    }
}

void MemoryTab::onCreateClicked() {
    if (!m_memMgr) return;

    bool ok;
    QString name = QInputDialog::getText(this, "Create Memory Module", "Memory File Name (e.g. topic_notes.md):", QLineEdit::Normal, "", &ok);
    if (ok && !name.trimmed().isEmpty()) {
        QString fileName = name.trimmed();
        if (!fileName.endsWith(".md")) fileName += ".md";

        m_memMgr->createMemory(fileName, QString("# %1\n\n- Add memory details here...\n").arg(fileName));
        m_activeFileName = fileName;
        refreshMemoryList();
    }
}

void MemoryTab::onDeleteClicked() {
    if (m_activeFileName.isEmpty() || !m_memMgr) return;

    QMessageBox::StandardButton res = QMessageBox::warning(
        this,
        "Delete Memory File",
        QString("Are you sure you want to delete memory module '.memory/%1'?").arg(m_activeFileName),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (res == QMessageBox::Yes) {
        m_memMgr->deleteMemory(m_activeFileName);
        m_activeFileName.clear();
        refreshMemoryList();
    }
}

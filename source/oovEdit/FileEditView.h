/*
 * FileEditView.h
 *
 *  Created on: Oct 26, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef FILEEDITVIEW_H_
#define FILEEDITVIEW_H_


#include "Highlighter.h"
#include "Indenter.h"
#include "History.h"


class CompletionList
    {
    public:
	CompletionList():
	    mEditView(nullptr), mTopWidget(nullptr), mTextBuffer(nullptr),
	    mStartIdentifierOffset(0), mCompletionTriggerPointOffset(0),
	    mLastKey(0), mLastNonModifierKey(0)
	    {}
	void init(GtkTextBuffer *textBuffer);
	void positionCompletionWindow();
	// Detects when to display the completion list.
	// Return indicates to start getting completion data.
	bool handleEditorKey(int key, int modKeys);
	bool handleListKey(int key, int modKeys);
	void setList(class FileEditView *view, OovStringVec const &strs);
	void activateSelectedItem();
	void lostFocus();
	int getCompletionTriggerPointOffset()
	    { return mCompletionTriggerPointOffset; }

    private:
	FileEditView *mEditView;
	GtkWidget *mTopWidget;
	GuiList mGuiList;
	GtkTextBuffer *mTextBuffer;
	int mStartIdentifierOffset;
	int mCompletionTriggerPointOffset;
	int mLastKey;
	int mLastNonModifierKey;
	void setWindowPosition(GdkWindow *compWin, int screenWidth);
    };

class FileEditView
    {
    public:
	FileEditView():
	    mTextView(nullptr), mTextBuffer(nullptr), mCurHistoryPos(0), mDoingHistory(false)
	    {}
	void init(GtkTextView *textView);
	bool openTextFile(OovStringRef const fn);
	bool saveTextFile();
	bool saveAsTextFileWithDialog();
	bool checkExitSave();
	std::string getSelectedText();
	bool find(char const * const findStr, bool forward, bool caseSensitive);
	bool findAndReplace(char const * const findStr, bool forward,
		bool caseSensitive, char const * const replaceStr);
	void findToken(eFindTokenTypes ft)
            { mHighlighter.findToken(ft, GuiTextBuffer::getCursorOffset(mTextBuffer)); }
        void getFindTokenResults(std::string &fn, int &offset)
            { mHighlighter.getFindTokenResults(fn, offset); }
	void cut()
	    {
	    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	    gtk_text_buffer_cut_clipboard(mTextBuffer, clipboard, true);
	    }
	void copy()
	    {
	    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	    gtk_text_buffer_copy_clipboard(mTextBuffer, clipboard);
	    }
	void paste()
	    {
	    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	    gtk_text_buffer_paste_clipboard(mTextBuffer, clipboard, NULL, true);
	    }
	void deleteSel()
	    {
	    gtk_text_buffer_delete_selection(mTextBuffer, true, true);
	    }
	void undo()
	    {
	    mDoingHistory = true;
	    if(mCurHistoryPos > 0)
		mHistoryItems[--mCurHistoryPos].undo(mTextBuffer);
	    mDoingHistory = false;
	    }
	void redo()
	    {
	    mDoingHistory = true;
	    if(mCurHistoryPos < mHistoryItems.size())
		mHistoryItems[mCurHistoryPos++].redo(mTextBuffer);
	    mDoingHistory = false;
	    }
	void addHistoryItem(const HistoryItem &item)
	    {
	    if(mCurHistoryPos < mHistoryItems.size())
		mHistoryItems[mCurHistoryPos] = item;
	    else
		mHistoryItems.push_back(item);
	    mCurHistoryPos++;
	    }
	bool doingHistory() const
	    { return mDoingHistory; }
	GtkTextView *getTextView() const
	    { return mTextView; }
	GtkWindow *getWindow()
	    {
	    GtkWindow *wnd = nullptr;
	    if(mTextView)
		wnd = GTK_WINDOW(mTextView);
	    return wnd;
	    }
        // Return = true if find def/decl has results.
	bool idleHighlight();
	void gotoLine(int lineNum);

	// These are called by callbacks.
	void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
	        gchar *text, gint len);
	void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
	        GtkTextIter *end);
	bool handleIndentKeys(GdkEvent *event);
	std::string getFilename() const
	    { return mFileName; }
	GtkTextBuffer *getTextBuffer() const
	    { return mTextBuffer; }

    private:
	std::string mFileName;
	GtkTextView *mTextView;
	GtkTextBuffer *mTextBuffer;
	std::vector<HistoryItem> mHistoryItems;
	size_t mCurHistoryPos;		// Range is 0 to size()-1.
	bool mDoingHistory;		// Doing undo or redo.
	Highlighter mHighlighter;
	Indenter mIndenter;
	CompletionList mCompleteList;
	void setFileName(OovStringRef const fn)
	    { mFileName = fn; }
	bool saveAsTextFile(OovStringRef const fn);
	void highlightRequest();
	void moveToIter(GtkTextIter startIter, GtkTextIter *endIter=NULL);
	GuiText getBuffer();
    };


#endif /* FILEEDITVIEW_H_ */

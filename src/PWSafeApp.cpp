/* Copyright 2020 Ian Boisvert */
#include <time.h>
#include <algorithm>

#ifdef HAVE_GLOG
#include "libglog.h"
#endif

#include "AccountsWin.h"
#include "Dialog.h"
#include "Label.h"
#include "MessageBox.h"
#include "PWSafeApp.h"
#include "SafeCombinationPromptDlg.h"
#include "Utils.h"
#include "ResultCode.h"

const char *PWSafeApp::APPNAME_VERSION = NCPWSAFE_APPNAME " " NCPWSAFE_VERSION;

void PWSafeApp::Init(ProgArgs args)
{
    args_ = args;

    prefs_.ReadPrefs(args_.config_file_);

    db_.ReadOnly() = args.read_only_;

    // Get progname
    std::string prog_name = args.prog_name_;
    int pos = prog_name.rfind(L'/');
    if (pos > -1)
    {
        prog_name = prog_name.substr(pos + 1);
    }

    std::string &pathname = args.database_;
    // if pathname passed in command line, it takes precedence
    // over that in preference:
    if (pathname.empty())
    {
        pathname = prefs_.GetPrefValue<std::string>(Prefs::DB_PATHNAME);
    }
    if (!pathname.empty())
    {
        db_.DbPathname() = pathname;
    }

    db_.Password() = args.password_;
}

/** Open and edit an account database */
DialogResult PWSafeApp::Show()
{
    InitTUI();

    // Command bar must be constructed first
    commandbarwin_ = std::make_unique<CommandBarWin>(*this, commandbar_win_);
    accountswin_ = std::make_unique<AccountsWin>(*this, win_);

    wrefresh(root_win_);

    // prompt for password, try to Load.
    SafeCombinationPromptDlg pwdprompt(*this);

    // If a password was given on the command line and the database is known,
    // from the command line or the preferences, try them first and only
    // prompt if they don't work
    bool try_args = !db_.DbPathname().empty() && !db_.Password().empty();
    if (!db_.Password().empty() && db_.DbPathname().empty())
    {
        MessageBox(*this).Show(win_, "A password was given on the command line, "
            "but no account database was specified");
        ZeroString(db_.Password());
    }

    DialogResult dr = DialogResult::CONTINUE;
    while (dr != DialogResult::OK && dr != DialogResult::CANCEL)
    {
        std::string db_pathname, password;
        if (try_args)
        {
            try_args = false;
            db_pathname = db_.DbPathname();
            password = db_.Password();
            std::string msg = SafeCombinationPromptDlg::CheckDatabase(db_, db_pathname, password,
                "The password given on the command line is incorrect");
            if (msg.empty())
            {
                dr = DialogResult::OK;
            }
            else
            {
                MessageBox(*this).Show(win_, msg.c_str());
                ZeroString(db_.Password());
                ZeroString(password);
                continue;
            }
        }
        else
        {
            dr = pwdprompt.Show(win_);
            if (dr == DialogResult::OK)
            {
                db_pathname = pwdprompt.GetFilename();
                password = pwdprompt.GetPassword();
            }
        }

        if (dr == DialogResult::OK)
        {
            wrefresh(win_);

            db_.DbPathname() = db_pathname;
            db_.Password() = password;

            if (db_.CheckPassword())
            {
                if (db_.ReadDb())
                {
                    // Save the database file that was opened
                    prefs_.Set(Prefs::DB_PATHNAME, db_pathname);

                    db_.ClearDirty();
                    dr = accountswin_->Show();
                }
                else
                {
                    std::string msg("An error occurred reading database file ");
                    msg.append(db_pathname.c_str());
                    MessageBox(*this).Show(win_, msg.c_str());
                    // Prompt again instead of exiting
                    dr = DialogResult::CONTINUE;
                }
            }
        }
    }

    EndTUI();

    return dr;
}

/**
 * Save the database.
 * Backup the database first if enabled in preferences
 */
int PWSafeApp::Save()
{
    int rc = RC_SUCCESS;
    if (db_.IsDirty())
    {
        if (prefs_.GetPrefValue<bool>(Prefs::BACKUP_BEFORE_SAVE))
        {
            BackupDb();
        }
        db_.WriteDb(&rc);
    }
    return rc;
}

/** Save state */
void PWSafeApp::SavePrefs()
{
    prefs_.WritePrefs(args_.config_file_);
}

void PWSafeApp::InitTUI()
{
    /* Initialize curses */
    root_win_ = initscr();
    theme_.Init(prefs_.Get<std::string>(Prefs::THEME, ""));
    raw();
    noecho();
    // IMB 2022-12-13 htop uses F3/S-F3 to find next/prev, like in
    //   windows. But S-F3 doesn't work with the "screen" terminal,
    //   which seems to be the default terminal set by tmux,
    //   one needs to use something else like "screen.vte" so that
    //   the correct ansi code is generated.
    //   Instead of requiring a non-default setting we will use
    //   different keys--IDEA uses ^L for example.
    // define_key("\033[14;2~", KEY_F(15));  // Shift-F3

    save_cursor_ = curs_set(/*visibility*/ 1);

    // IMB 2022-12-24 ncurses behaves strangely--
    // max_y in this case is the line below the bottom line on the screen
    // This behaviour is probably documented somewhere..
    int max_y, max_x, beg_y, beg_x;
    getmaxyx(root_win_, max_y, max_x);
    getbegyx(root_win_, beg_y, beg_x);
    int nlines = max_y - beg_y, ncols = max_x - beg_x;

    accounts_win_ = derwin(root_win_, nlines - 1, ncols, /*begin_y*/ 0, /*begin_x*/ 0);

    win_ = newwin(nlines - 3, ncols - 2, /*begin_y*/ beg_y + 1, /*begin_x*/ beg_x + 1);

    commandbar_win_ = newwin(/*nlines*/ 1, ncols, /*begin_y*/ nlines - 1, /*begin_x*/ beg_x);
    commandbar_panel_ = new_panel(commandbar_win_);

    DrawFrame();

}

/** Draw the border and title of the accounts list */
void PWSafeApp::DrawFrame()
{
    werase(accounts_win_);
    box(accounts_win_, /*verch*/ 0, /*horch*/ 0);
    const std::string title = std::string(" ") + APPNAME_VERSION + " ";
    Label::Write(accounts_win_, /*y*/ 0, /*x*/ 0, getmaxx(accounts_win_), title.c_str(), theme_.Attr(Role::TITLE),
                 JUSTIFY_CENTER);
    // accounts_win_ is a subwindow of stdscr, which is the bottom panel
    touchwin(root_win_);
}

/**
 * Resize the application windows to fit the terminal.
 * When this is called, ncurses has already resized stdscr to the new terminal size.
 */
void PWSafeApp::LayoutTUI()
{
    int nlines = getmaxy(root_win_), ncols = getmaxx(root_win_);

    wresize(accounts_win_, nlines - 1, ncols);
    DrawFrame();

    // Position of the accounts list content window doesn't change, only its size.
    // Owners of the content of win_ re-draw it from their resize handlers.
    wresize(win_, nlines - 3, ncols - 2);

    // If the terminal shrank, the command bar may now be below the bottom of the screen.
    // ncurses has already narrowed it, so it can be moved before it is resized.
    move_panel(commandbar_panel_, /*starty*/ nlines - 1, /*startx*/ 0);
    wresize(commandbar_win_, /*nlines*/ 1, ncols);
    werase(commandbar_win_);
    if (commandbarwin_)
    {
        commandbarwin_->Redraw();
    }
}

void PWSafeApp::HandleResize()
{
    // If the terminal is too small, leave windows as they were truncated by ncurses.
    // They will be laid out again when the terminal is made large enough.
    if (getmaxy(root_win_) >= MIN_TERM_LINES && getmaxx(root_win_) >= MIN_TERM_COLS)
    {
        LayoutTUI();

        // Handlers may not be added or removed while they are being called,
        // but use an index anyway to be robust against reallocation
        for (size_t i = 0; i < resize_handlers_.size(); ++i)
        {
            resize_handlers_[i]();
        }
    }

    // Terminals may leave garbage on the screen after resizing, repaint everything
    clearok(curscr, TRUE);
    update_panels();
    doupdate();
}

void PWSafeApp::EndTUI()
{
    delwin(win_);
    win_ = nullptr;
    del_panel(commandbar_panel_);
    commandbar_panel_ = nullptr;
    delwin(commandbar_win_);
    commandbarwin_ = nullptr;
    delwin(accounts_win_);
    accounts_win_ = nullptr;
    endwin();

    curs_set(save_cursor_);
    echo();
}

void PWSafeApp::DoSearch()
{
    update_panels();
    hide_panel(commandbar_panel_);

    SearchBarWin{*this, commandbar_win_, *accountswin_}.Show();

    show_panel(commandbar_panel_);
    update_panels();
    doupdate();
}

ResultCode PWSafeApp::BackupDb()
{
    ResultCode status = BackupDbImpl();
    if (status == RC_SUCCESS) CleanBackups();
    return status;
}

typedef std::vector<std::pair<std::string, std::string>> backup_list_t;

/**
 * Returns an iterator at which expired backup files should be removed.
 * Backups are assumed to be in order of decreasing age--
 * the last entry in the list was the most recent backup
*/
static backup_list_t::iterator FilterBackups(backup_list_t &backups, size_t backup_count)
{
    backup_list_t::iterator retval = backups.end();
    if (backups.size() > backup_count)
    {
        // Swap the most recent backups at the end of the list 
        // with the items at the beginning
        for (backup_list_t::iterator src = backups.end()-backup_count, dst = backups.begin(); src != backups.end(); ++src, ++dst)
        {
            swap(*dst, *src);
        }
        retval = backups.begin() + backup_count;
    }
    return retval;
}

// Must be > 0. If Prefs::BACKUP_COUNT >= MAX_BACKUPS, 
// then eventually backups will stop being tracked
static constexpr size_t MAX_BACKUPS = 128;

// IMB 2023-08-27 This function seems much more complicated than it should be
void PWSafeApp::CleanBackups()
{
    namespace fs = std::filesystem;

    char key[32];
    long lval = prefs_.GetPrefValue<long>(Prefs::BACKUP_COUNT);
    size_t backup_count = lval > 0 ? lval : 0;

    backup_list_t backups;

    // Check that backup files saved in prefs actually exist
    size_t nbackup = 0;
    for ( ; nbackup < MAX_BACKUPS; ++nbackup)
    {
        snprintf(key, 32, "backup.%zu", nbackup);
        if (!prefs_.HasPref(key)) break;

        std::string pathname = prefs_.GetPrefValue<std::string>(key);
        if (fs::file_status fs = fs::status(pathname); fs.type() != fs::file_type::regular)
        {
            // File doesn't exist or isn't a regular file -> disown
            prefs_.Delete(key);
        }
        else
        {
            backups.emplace_back(key, pathname);
        }

    }

    // Filter the list of backups, 
    // after function call `pos` points to the start of backups to delete
    backup_list_t::iterator pos = FilterBackups(backups, backup_count);

    // First handle deletions because when preferences are updated, the keys will be modified
    for (backup_list_t::iterator it = pos; it != backups.end(); ++it)
    {
        std::error_code ec;
        filesystem_.Remove(it->second, ec);
#if HAVE_GLOG
        LOG_IF(WARNING, ec) << "Error deleting backup file \"" << it->second << "\": " << ec.message();
#endif
    }

    const size_t len = pos - backups.begin();
    for (nbackup = 0; nbackup != len; ++nbackup)
    {
        snprintf(key, 32, "backup.%zu", nbackup);
        prefs_.Set(key, backups[nbackup].second);
    }

    for ( ; nbackup < MAX_BACKUPS; ++nbackup)
    {
        snprintf(key, 32, "backup.%zu", nbackup);
        prefs_.Delete(key);
    }
}

/** Backup the current account database */
ResultCode PWSafeApp::BackupDbImpl()
{
    namespace fs = std::filesystem;

    const std::string &db = GetArgs().database_;
    if (db.empty()) return RC_ERR_BACKUP;  // Sanity check

    ResultCode status = RC_SUCCESS;

    fs::path db_path(filesystem_.Canonical(db));
    fs::path filename = db_path.filename();

    time_t now = time(nullptr);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    constexpr size_t buflen = 128;
    char buf[buflen];
    snprintf(buf, buflen, "-%04d%02d%02d_%02d%02d%02d", 
        tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);  // -YYYYMMdd_HHmmss.bak

    std::string backup_filename = db_path.stem().string() + buf + db_path.extension().string();
    fs::path backup = db_path.parent_path() / backup_filename;
    
    if (filesystem_.Exists(backup))
    {
        // IMB 2023-08-27 I don't think preserving an existing backup 
        // with identical timestamp is the right answer--overwrite and log
        // // Backup backup
        // fs::path backup2(backup);
        // backup2 += ".2";
        // try
        // {
        //     filesystem_.Copy(backup, backup2);
        // }
        // catch (fs::filesystem_error &cause)
        // {
        //     // TODO IMB 2023-07-01 log error copying backup file
        // }
        // backup += ".1";
#if HAVE_GLOG
        LOG(WARNING) << "A backup file with pathname \"" << backup << "\" already exists. Overwriting.";
#endif        
    }

    try
    {
        filesystem_.Copy(db_path, backup);

        // Append backup file pathname to prefs
        char key[32];
        size_t nbackup = 0;
        for ( ; nbackup < MAX_BACKUPS; ++nbackup)
        {
            snprintf(key, 32, "backup.%zu", nbackup);
            if (!prefs_.HasPref(key))
            {
                std::error_code ec;
                backup = filesystem_.Canonical(backup, ec);  // Ignore errors
                if (ec)
                {
#if HAVE_GLOG
                    LOG(WARNING) << "Error converting to canonical path \"" << backup.c_str() << "\": " << ec.message();
#endif                    
                }
                prefs_.Set(key, backup.string());
                break;
            }
        }
#if HAVE_GLOG
        long lval = prefs_.GetPrefValue<long>(Prefs::BACKUP_COUNT);
        size_t backup_count = lval > 0 ? lval : 0;
        LOG_IF(INFO, nbackup == MAX_BACKUPS) << "Too many account database backup files stored in preferences (" << MAX_BACKUPS << ")";
        LOG_IF(WARNING, backup_count >= MAX_BACKUPS) << "Preference \"backup-count\" is set too high (" << backup_count << "), it should be less than (" << MAX_BACKUPS << ")";
#endif        
    }
    catch (fs::filesystem_error &cause)
    {
#if HAVE_GLOG
        LOG(WARNING) << "Error copying account database file \"" << cause.path1() << "\" to backup file \"" << cause.path2() << "\": " << cause.what();
#endif
        status = RC_ERR_BACKUP;
    }

    return status;
}

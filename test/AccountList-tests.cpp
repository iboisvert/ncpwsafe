/* Copyright 2026 Ian Boisvert */
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "AccountList.h"

using Kind = AccountList::RowKind;

/** The accounts of test/e2e/fixtures/small.tsv, sorted by group and title */
static AccountRecords SmallRecords()
{
    return AccountRecords{
        AccountRecord{{FT_TITLE, "Router"}, {FT_USER, "admin"}},
        AccountRecord{{FT_TITLE, "Wiki"}, {FT_USER, "wikiuser"}},
        AccountRecord{{FT_GROUP, "Banking"}, {FT_TITLE, "Chequing"}, {FT_USER, "alice"}},
        AccountRecord{{FT_GROUP, "Banking"}, {FT_TITLE, "Savings"}, {FT_USER, "alice"}},
        AccountRecord{{FT_GROUP, "Email"}, {FT_TITLE, "Personal mail"}},
        AccountRecord{{FT_GROUP, "Email"}, {FT_TITLE, "Work mail"}},
        AccountRecord{{FT_GROUP, "Shopping"}, {FT_TITLE, "Bookstore"}},
    };
}

/** Each row as text: "# group (count)" for a heading, "+ group" if collapsed, the title for an account */
static std::vector<std::string> RowsText(const AccountList &list)
{
    std::vector<std::string> text;
    for (const AccountList::Row &row : list.Rows())
    {
        if (row.kind == Kind::HEADING)
            text.push_back(std::string(row.collapsed ? "+ " : "# ") + row.group + " (" + std::to_string(row.count) + ")");
        else
            text.push_back(row.record->GetField(FT_TITLE, ""));
    }
    return text;
}

/** The selected row as text, see RowsText() */
static std::string SelectedText(const AccountList &list)
{
    return list.Selected() < 0 ? "<none>" : RowsText(list)[static_cast<size_t>(list.Selected())];
}

TEST(AccountListTest, HeadingsForEachGroup)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    EXPECT_TRUE(list.HasHeadings());
    EXPECT_EQ((std::vector<std::string>{"#  (2)", "Router", "Wiki", "# Banking (2)", "Chequing", "Savings",
                                        "# Email (2)", "Personal mail", "Work mail", "# Shopping (1)", "Bookstore"}),
              RowsText(list));
}

TEST(AccountListTest, SelectsFirstAccount)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    EXPECT_EQ("Router", SelectedText(list));
    EXPECT_EQ(&*records.begin(), list.SelectedRecord());
}

TEST(AccountListTest, NoHeadingsWithoutGroups)
{
    AccountRecords records{AccountRecord{{FT_TITLE, "Router"}}, AccountRecord{{FT_TITLE, "Wiki"}}};
    AccountList list;
    list.Build(records);
    EXPECT_FALSE(list.HasHeadings());
    EXPECT_EQ((std::vector<std::string>{"Router", "Wiki"}), RowsText(list));
    // Left does nothing without headings
    list.Left();
    EXPECT_EQ("Router", SelectedText(list));
}

TEST(AccountListTest, EmptyList)
{
    AccountRecords records;
    AccountList list;
    list.Build(records);
    EXPECT_TRUE(list.Rows().empty());
    EXPECT_EQ(-1, list.Selected());
    EXPECT_EQ(nullptr, list.SelectedRow());
    EXPECT_EQ(nullptr, list.SelectedRecord());
    list.MoveDown();
    list.MoveUp();
    list.MoveLast();
    list.Left();
    list.Right();
    EXPECT_FALSE(list.Toggle());
    EXPECT_EQ(-1, list.Selected());
    EXPECT_EQ(0, list.ScrollTop(10));
}

TEST(AccountListTest, MovesStopAtEnds)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.MoveUp();
    EXPECT_EQ("#  (2)", SelectedText(list));
    list.MoveUp();
    EXPECT_EQ("#  (2)", SelectedText(list));
    list.MoveLast();
    EXPECT_EQ("Bookstore", SelectedText(list));
    list.MoveDown();
    EXPECT_EQ("Bookstore", SelectedText(list));
    list.MoveFirst();
    EXPECT_EQ("#  (2)", SelectedText(list));
}

TEST(AccountListTest, PageMovesClampToEnds)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.MovePageDown(3);
    EXPECT_EQ("Chequing", SelectedText(list));
    list.MovePageDown(100);
    EXPECT_EQ("Bookstore", SelectedText(list));
    list.MovePageUp(2);
    EXPECT_EQ("Work mail", SelectedText(list));
    list.MovePageUp(100);
    EXPECT_EQ("#  (2)", SelectedText(list));
}

TEST(AccountListTest, CollapseAndExpand)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.SelectRecord(*(records.begin() + 3));  // Savings
    // Left goes to the heading, then collapses the group
    list.Left();
    EXPECT_EQ("# Banking (2)", SelectedText(list));
    list.Left();
    EXPECT_EQ("+ Banking (2)", SelectedText(list));
    EXPECT_TRUE(list.IsCollapsed("Banking"));
    EXPECT_EQ((std::vector<std::string>{"#  (2)", "Router", "Wiki", "+ Banking (2)", "# Email (2)", "Personal mail",
                                        "Work mail", "# Shopping (1)", "Bookstore"}),
              RowsText(list));
    // Down from a collapsed heading skips its accounts
    list.MoveDown();
    EXPECT_EQ("# Email (2)", SelectedText(list));
    list.MoveUp();
    // Right expands, then goes to the first account
    list.Right();
    EXPECT_EQ("# Banking (2)", SelectedText(list));
    EXPECT_FALSE(list.IsCollapsed("Banking"));
    list.Right();
    EXPECT_EQ("Chequing", SelectedText(list));
}

TEST(AccountListTest, ToggleOnlyOnHeadings)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    EXPECT_FALSE(list.Toggle());  // Router
    list.MoveUp();
    EXPECT_TRUE(list.Toggle());
    EXPECT_EQ((std::vector<std::string>{"+  (2)", "# Banking (2)", "Chequing", "Savings", "# Email (2)",
                                        "Personal mail", "Work mail", "# Shopping (1)", "Bookstore"}),
              RowsText(list));
    EXPECT_TRUE(list.Toggle());
    EXPECT_EQ(11u, list.Rows().size());
}

TEST(AccountListTest, SelectRecordExpandsItsGroup)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.MoveLast();
    list.Left();
    list.Left();  // Collapse Shopping
    ASSERT_TRUE(list.IsCollapsed("Shopping"));
    ASSERT_TRUE(list.SelectRecord(*(records.begin() + 6)));
    EXPECT_EQ("Bookstore", SelectedText(list));
    EXPECT_FALSE(list.IsCollapsed("Shopping"));

    AccountRecord other{{FT_TITLE, "Other"}};
    EXPECT_FALSE(list.SelectRecord(other));
}

TEST(AccountListTest, BuildKeepsCollapsedGroups)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.MoveLast();
    list.Left();
    list.Left();  // Collapse Shopping
    list.Build(records);
    EXPECT_TRUE(list.IsCollapsed("Shopping"));
    EXPECT_EQ("+ Shopping (1)", RowsText(list).back());
}

TEST(AccountListTest, PositionOfHeadingAndAccount)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    list.MoveDown();
    list.MoveDown();  // Banking
    AccountList::Position heading = list.GetPosition();
    list.MoveLast();
    AccountList::Position account = list.GetPosition();
    list.SetPosition(heading);
    EXPECT_EQ("# Banking (2)", SelectedText(list));
    list.SetPosition(account);
    EXPECT_EQ("Bookstore", SelectedText(list));
}

TEST(AccountListTest, ScrollTopKeepsSelectionVisible)
{
    AccountRecords records = SmallRecords();
    AccountList list;
    list.Build(records);
    // Everything fits
    EXPECT_EQ(0, list.ScrollTop(20));
    list.MoveLast();  // Row 10
    EXPECT_EQ(7, list.ScrollTop(4));
    // Moving up within the visible rows doesn't scroll
    list.MoveUp();
    list.MoveUp();
    EXPECT_EQ(7, list.ScrollTop(4));
    list.MoveFirst();
    EXPECT_EQ(0, list.ScrollTop(4));
}

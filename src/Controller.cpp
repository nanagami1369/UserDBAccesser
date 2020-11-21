#include "Controller.h"
#include "PromptModule.h"
#include "UserDB.h"
#include "ValidationException.h"
#include <iostream>
#include <regex>
#include <termios.h>
#include <unistd.h>

Controller::Controller(UserDB *db) {
    this->db = db;
}

Controller::~Controller() {
    delete db;
}

uint Controller::InputIdPrompt() {
    uint lastId = -1;
    try {
        lastId = db->GetLastId();
    } catch (const std::range_error &e) {
        throw;
    }
    while (true) {
        try {
            return Prompt::inputNumberPrompt("IDを入力して下さい", 0, lastId);
        } catch (const ValidationException &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

void Controller::addUser() {
    std::cout << "ユーザーを追加します" << std::endl;
    // 名前入力
    std::string name = "";
    while (true) {
        try {
            name = Prompt::inputStringPrompt("名前を入力して下さい");
            break;
        } catch (const ValidationException &e) {
            std::cerr << e.what() << std::endl;
        }
    }
    // パスワード入力
    std::string pass = Prompt::inputPasswordPrompt();
    // 有効、無効
    uint8_t availNumber;
    constexpr int availMenuLength = 2;
    const char *availMenu[availMenuLength] = {"有効", "無効"};
    availNumber = Prompt::selectMenuPrompt("どの状態で作成しますか", availMenu, availMenuLength);
    auto avail = availNumber == 0 ? true : false;
    // 権限
    uint8_t levelNumber;
    constexpr int levelMenuLength = 4;
    const char *levelMenu[levelMenuLength] = {"ADMIN", "PREM", "GEN", "TRY"};
    levelNumber = Prompt::selectMenuPrompt("権限を決定して下さい", levelMenu, levelMenuLength);
    auto level = intToT_Level(levelNumber);
    std::cout << "==================================" << std::endl;
    std::cout << "氏名：" << name << std::endl;
    std::cout << "パスワード：********" << std::endl;
    std::cout << "アカウントの状態：" << (avail ? "有効" : "無効") << std::endl;
    std::cout << "権限：" << t_LevelToString(level) << std::endl;
    std::cout << "==================================" << std::endl;
    const char *isSaveUserMenu[] = {"追加する", "いいえ"};
    auto isSave = Prompt::yesOrNoPrompt("以下のアカウントを追加しますか？", isSaveUserMenu);
    if (isSave) {
        db->add(name, pass, avail, level);
        std::cout << "ユーザーの追加に成功しました" << std::endl;
        return;
    }
    std::cout << "ユーザーの追加を止めました" << std::endl;
    return;
}

void Controller::removeUser() {
    uint id = -1;
    try {
        id = InputIdPrompt();
    } catch (const std::range_error &e) {
        std::cerr << e.what() << std::endl;
        return;
    }
    try {
        auto user = this->db->searchById(id);
        constexpr int yesOrNoMenuLength = 2;
        const char *yesOrNoMenu[yesOrNoMenuLength] = {"削除する", "いいえ"};
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << user.toString() << std::endl;
        std::cout << "==================================" << std::endl;
        auto select = Prompt::yesOrNoPrompt("このアカウントを削除しますか？", yesOrNoMenu);
        if (select) {
            db->remove(user.ID);
            std::cout << "アカウントを削除しました。" << std::endl;
            return;
        }
        std::cout << "アカウントを削除しませんでした" << std::endl;
    } catch (const ValidationException &e) {
        std::cerr << e.what() << std::endl;
    } catch (const std::range_error &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Controller::changeAvail() {
    uint id = -1;
    try {
        id = InputIdPrompt();
    } catch (const std::range_error &e) {
        std::cerr << e.what() << std::endl;
        return;
    }
    try {
        auto user = db->searchById(id);
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << user.toString() << std::endl;
        std::cout << "==================================" << std::endl;
        if (user.Avail) {
            std::cout << "このアカウントは有効です" << std::endl;
            const char *unEnableYesOrNoMenu[] = {"無効化する", "いいえ"};
            auto select = Prompt::yesOrNoPrompt("無効化しますか？", unEnableYesOrNoMenu);
            if (select) {
                try {
                    db->update(user.ID, user.Name, "", false, user.Level);
                } catch (const std::range_error &e) {
                    std::cerr << "アカウントが見つかりませんでした" << std::endl;
                } catch (const ValidationException &e) {
                    std::cerr << e.what() << std::endl;
                }
                std::cout << "アカウントを無効化しました" << std::endl;
                return;
            }
            std::cout << "アカウントを無効化しませんでした" << std::endl;
        } else {
            std::cout << "このアカウントは無効化されています" << std::endl;
            const char *enableYesOrNoMenu[] = {"有効にする", "いいえ"};
            auto select = Prompt::yesOrNoPrompt("有効にしますか？", enableYesOrNoMenu);
            if (select) {
                try {
                    db->update(user.ID, user.Name, "", true, user.Level);
                } catch (const std::range_error &e) {
                    std::cerr << "アカウントが見つかりませんでした" << std::endl;
                } catch (const ValidationException &e) {
                    std::cerr << e.what() << std::endl;
                }
                std::cout << "アカウントを有効にしました" << std::endl;
                return;
            }
            std::cout << "アカウントを有効にしませんでした" << std::endl;
        }
    } catch (const std::range_error &e) {
        std::cerr << "アカウントが見つかりませんでした" << std::endl;
        return;
    } catch (const ValidationException &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Controller::searchUser() {
    std::cout << "検索を開始します" << std::endl;
    constexpr int searchModeMenuLength = 4;
    const char *searchModeMenu[searchModeMenuLength] = {
        "IDで検索する",
        "名前で検索する",
        "アカウントの状態で検索する",
        "権限で検索する"};
    auto searchMode = Prompt::selectMenuPrompt("検索モードを指定して下さい", searchModeMenu, searchModeMenuLength);
    switch (searchMode) {
    case 0:
        searchUserById();
        break;
    case 1:
        searchUserByName();
        break;
    case 2:
        searchUserByAvail();
        break;
    case 3:
        searchUserByLevel();
        break;
    default:
        break;
    }
}

void Controller::searchUserById() {
    std::cout << "IDで検索します" << std::endl;
    uint id = -1;
    try {
        id = InputIdPrompt();
    } catch (const std::range_error &e) {
        std::cerr << e.what() << std::endl;
        return;
    }
    try {
        auto user = db->searchById(id);
        constexpr int yesOrNoMenuLength = 2;
        const char *yesOrNoMenu[yesOrNoMenuLength] = {"はい", "いいえ"};
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << user.toString() << std::endl;
        std::cout << "==================================" << std::endl;
    } catch (const std::range_error &e) {
        std::cerr << "アカウントが見つかりませんでした" << std::endl;
        return;
    } catch (const ValidationException &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Controller::searchUserByName() {
    std::cout << "名前で検索します" << std::endl;
    std::string name = "";
    while (true) {
        try {
            name = Prompt::inputStringPrompt("名前を入力して下さい");
            break;
        } catch (const ValidationException &e) {
            std::cerr << e.what() << std::endl;
        }
    }
    try {
        auto users = db->searchByName(name);
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        for (auto &user : users) {
            std::cout << user.toString() << std::endl;
        }
        std::cout << "==================================" << std::endl;
    } catch (const std::range_error &e) {
        std::cerr << "アカウントが見つかりませんでした" << std::endl;
    }
}
void Controller::searchUserByAvail() {
    std::cout << "アカウントの状態で検索します" << std::endl;
    constexpr int searchAvailModeMenuLength = 2;
    const char *searchAvailModeMenu[searchAvailModeMenuLength] = {"有効", "無効"};
    auto searchAvailModeNumber = Prompt::selectMenuPrompt("どちらの状態を検索しますか？", searchAvailModeMenu, searchAvailModeMenuLength);
    auto searchAvailMode = searchAvailModeNumber == 0 ? true : false;
    if (searchAvailMode) {
        std::cout << "有効なアカウントを検索します" << std::endl;
    } else {
        std::cout << "無効なアカウントを検索します" << std::endl;
    }
    try {
        auto users = db->searchByAvail(searchAvailMode);
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        for (auto &user : users) {
            std::cout << user.toString() << std::endl;
        }
        std::cout << "==================================" << std::endl;
    } catch (const std::range_error &e) {
        std::cerr << "アカウントが見つかりませんでした" << std::endl;
    }
}

void Controller::searchUserByLevel() {
    std::cout << "権限で検索します" << std::endl;
    constexpr int searchLevelModeMenuLength = 4;
    const char *searchLevelModeMenu[searchLevelModeMenuLength] = {"ADMIN", "PREM", "GEN", "TRY"};
    auto searchLevel = Prompt::selectMenuPrompt("どちらの状態を検索しますか？", searchLevelModeMenu, searchLevelModeMenuLength);
    auto level = intToT_Level(searchLevel);
    std::cout << "この権限を持つアカウントを検索します-->" << t_LevelToString(level) << std::endl;
    try {
        auto users = db->searchByLevel(level);
        std::cout << "アカウントが見つかりました" << std::endl;
        std::cout << "==================================" << std::endl;
        for (auto &user : users) {
            std::cout << user.toString() << std::endl;
        }
        std::cout << "==================================" << std::endl;
    } catch (const std::range_error &e) {
        std::cerr << "アカウントが見つかりませんでした" << std::endl;
    }
}

void Controller::start() {
    std::cout << "UserDBへようこそ!!" << std::endl;

    int controlNumber = 100;
    constexpr int mainMenuLength = 6;
    const char *mainMenu[mainMenuLength] = {
        "終了",
        "ユーザーの追加",
        "ユーザーの一覧",
        "ユーザーの削除",
        "ユーザーの無効化、有効化",
        "ユーザーの検索"};
    while (controlNumber != 0) {
        controlNumber = Prompt::selectMenuPrompt("何をしますか？", mainMenu, mainMenuLength);
        switch (controlNumber) {
        case 0:
            std::cout << "さようなら!" << std::endl;
            break;
        case 1:
            addUser();
            break;
        case 2:
            std::cout << "ユーザーの一覧を表示します" << std::endl;
            db->WriterAllUserToConsole();
            Prompt::pausePrompt();
            break;
        case 3:
            removeUser();
            break;
        case 4:
            changeAvail();
            break;
        case 5:
            searchUser();
            break;
        default:
            std::cerr << "無効な数字です" << std::endl;
            break;
        }
    }
}

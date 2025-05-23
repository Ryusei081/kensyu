#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <locale.h>
#include <windows.h>

void show_subjects() {
    printf("\n--- 科目ID一覧 ---\n");
    printf("1: 国語\n2: 数学\n3: 英語\n4: 日本史\n5: 世界史\n6: 地理\n7: 物理\n8: 化学\n9: 生物\n");
}

int main() {
    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("update.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "DB接続失敗: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "");

    wchar_t wname[100];  // ワイド文字列（日本語入力用）
    char name[100];      // UTF-8に変換した名前（SQLite用）

    printf("受験者氏名を入力: ");
    fgetws(wname, sizeof(wname) / sizeof(wchar_t), stdin);

    // 改行除去
    size_t wlen = wcslen(wname);
    if (wlen > 0 && wname[wlen - 1] == L'\n') {
        wname[wlen - 1] = L'\0';
    }

    // UTF-8に変換
    WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, sizeof(name), NULL, NULL);

    // デバッグ表示
    printf("入力された名前（デバッグ）: [%s]\n", name);

    // applicant_id取得
    const char *sql_applicant = "SELECT applicant_id FROM applicants WHERE name = ?";
    rc = sqlite3_prepare_v2(db, sql_applicant, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);

    int applicant_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        applicant_id = sqlite3_column_int(stmt, 0);
    } else {
        printf("受験者が見つかりません\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    sqlite3_finalize(stmt);

    // 試験日入力
    char exam_date[9];
    printf("試験日(8桁 yyyymmdd): ");
    scanf("%8s", exam_date);

    // セッションID取得 or 作成
    int session_id = -1;
    const char *sql_session = "SELECT session_id FROM exam_sessions WHERE applicant_id = ? AND exam_date = ?";
    rc = sqlite3_prepare_v2(db, sql_session, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, applicant_id);
    sqlite3_bind_text(stmt, 2, exam_date, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        session_id = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (session_id == -1) {
        const char *sql_insert_session = "INSERT INTO exam_sessions (applicant_id, exam_date) VALUES (?, ?)";
        rc = sqlite3_prepare_v2(db, sql_insert_session, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }
        sqlite3_bind_int(stmt, 1, applicant_id);
        sqlite3_bind_text(stmt, 2, exam_date, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "session挿入失敗: %s\n", sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
        sqlite3_finalize(stmt);
        session_id = (int)sqlite3_last_insert_rowid(db);
    }

    int subject_id, score;
    show_subjects();
    printf("科目IDを入力: ");
    scanf("%d", &subject_id);
    printf("点数を入力: ");
    scanf("%d", &score);

    // exam_resultsにsession_idがなければ追加
    const char *sql_check = "SELECT session_id FROM exam_results WHERE session_id = ?";
    rc = sqlite3_prepare_v2(db, sql_check, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, session_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_ROW) {
        const char *sql_insert_result = "INSERT INTO exam_results (session_id) VALUES (?)";
        rc = sqlite3_prepare_v2(db, sql_insert_result, -1, &stmt, NULL);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            return 1;
        }
        sqlite3_bind_int(stmt, 1, session_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    // 点数更新
    const char *sql_update = NULL;
    switch (subject_id) {
        case 1: sql_update = "UPDATE exam_results SET subject1_id = 1, subject1_score = ? WHERE session_id = ?"; break;
        case 2: sql_update = "UPDATE exam_results SET subject2_id = 2, subject2_score = ? WHERE session_id = ?"; break;
        case 3: sql_update = "UPDATE exam_results SET subject3_id = 3, subject3_score = ? WHERE session_id = ?"; break;
        case 4: sql_update = "UPDATE exam_results SET subject4_id = 4, subject4_score = ? WHERE session_id = ?"; break;
        case 5: sql_update = "UPDATE exam_results SET subject5_id = 5, subject5_score = ? WHERE session_id = ?"; break;
        default:
            printf("対応していない科目です。\n");
            sqlite3_close(db);
            return 0;
    }

    rc = sqlite3_prepare_v2(db, sql_update, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "準備失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, session_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "点数更新失敗: %s\n", sqlite3_errmsg(db));
    } else {
        printf("点数が更新されました。\n");
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return 0;
}

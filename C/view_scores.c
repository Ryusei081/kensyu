#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <windows.h>
#include <sqlite3.h>

void show_score(int subject_id, int score) {
    const char* subject_name = "不明";
    switch (subject_id) {
        case 1: subject_name = "国語"; break;
        case 2: subject_name = "数学"; break;
        case 3: subject_name = "英語"; break;
        case 4: subject_name = "日本史"; break;
        case 5: subject_name = "世界史"; break;
        case 6: subject_name = "地理"; break;
        case 7: subject_name = "物理"; break;
        case 8: subject_name = "化学"; break;
        case 9: subject_name = "生物"; break;
    }
    if (subject_id > 0)
        printf("  %s（ID:%d）: %d点\n", subject_name, subject_id, score);
}

int main() {
    setlocale(LC_ALL, "Japanese_Japan.932");

    sqlite3 *db;
    sqlite3_stmt *stmt;
    int rc = sqlite3_open("update.db", &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "DB接続失敗: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    char name[100];
    printf("受験者氏名を入力: ");
    fgets(name, sizeof(name), stdin);
    size_t len = strlen(name);
    if (len > 0 && (name[len - 1] == '\n' || name[len - 1] == '\r')) name[--len] = '\0';

    // applicant_id取得
    const char *sql = "SELECT applicant_id FROM applicants WHERE name = ?";
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
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
        printf("受験者が見つかりません。\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 0;
    }
    sqlite3_finalize(stmt);

    // 試験データ取得
    const char *sql_scores =
        "SELECT s.exam_date, "
        "r.subject1_id, r.subject1_score, "
        "r.subject2_id, r.subject2_score, "
        "r.subject3_id, r.subject3_score, "
        "r.subject4_id, r.subject4_score, "
        "r.subject5_id, r.subject5_score "
        "FROM exam_sessions s "
        "JOIN exam_results r ON s.session_id = r.session_id "
        "WHERE s.applicant_id = ?";

    rc = sqlite3_prepare_v2(db, sql_scores, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "点数取得準備失敗: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    sqlite3_bind_int(stmt, 1, applicant_id);
    printf("\n--- 試験結果 ---\n");
    printf("受験者氏名: %s\n", name);
    int found = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        found = 1;
        const unsigned char *exam_date = sqlite3_column_text(stmt, 0);
        printf("試験日: %s\n", exam_date);
        for (int i = 1; i <= 9; i += 2) {
            int subject_id = sqlite3_column_int(stmt, i);
            int score = sqlite3_column_int(stmt, i + 1);
            if (subject_id != 0)
                show_score(subject_id, score);
        }
        printf("\n");
    }

    if (!found) {
        printf("この受験者の試験結果は登録されていません。\n");
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return 0;
}

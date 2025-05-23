#include <stdio.h>
#include <mysql.h>

int main() {
    printf("MySQL C 接続テスト開始\n");

    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        printf("mysql_init() 失敗\n");
        return 1;
    }

    // 接続：パスワードやDB名は実際の情報に書き換えてください
    if (mysql_real_connect(conn, "localhost", "root", "your_password", "testdb", 0, NULL, 0) == NULL) {
        printf("接続失敗: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    printf("MySQLに接続成功しました！\n");

    mysql_close(conn);
    return 0;
}

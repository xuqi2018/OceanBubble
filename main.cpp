#include <QApplication>

#include "ui/GameWindow.h"

// =============================================================================
// 海洋消消泡 —— 入口
// Qt Widgets 应用：主窗口负责布局，游戏逻辑位于 core/，绘制与交互位于 ui/。
// =============================================================================
int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt 5：在创建 QApplication 之前开启高 DPI 支持，避免高分屏下界面模糊。
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);

    // QSettings 使用「组织 + 应用」定位配置存储位置，用于持久化历史最高分。
    QCoreApplication::setOrganizationName(QStringLiteral("xuqi"));
    QCoreApplication::setApplicationName(QStringLiteral("OceanBubble"));

    GameWindow window;
    window.show();

    return app.exec();
}

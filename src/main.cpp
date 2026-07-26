#include "git/Repository.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QStringList>

int main(int argc, char* argv[])
{
    QCoreApplication::setApplicationName(QStringLiteral("GitScope"));
    QCoreApplication::setOrganizationName(QStringLiteral("GitScope"));
    QCoreApplication::setApplicationVersion(QStringLiteral(GITSCOPE_VERSION));

    QApplication app(argc, argv);

    // Initializes libgit2 and applies config-isolation hardening for the
    // whole process lifetime (see SECURITY.md).
    gitscope::git::LibGit2 libgit2;

    gitscope::ui::MainWindow window;
    window.show();

    const QStringList args = QApplication::arguments();
    if (args.size() > 1)
        window.openRepository(args.at(1));

    return QApplication::exec();
}

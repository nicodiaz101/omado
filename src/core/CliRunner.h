#pragma once

#include <QStringList>

class LocalRepository;
class DaemonService;

class CliRunner {
public:
    static int run(const QStringList &args, LocalRepository *repo);
};

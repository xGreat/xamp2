﻿#include <widget/database.h>

#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QThread>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/assert.h>
#include <widget/str_utilts.h>

#define THROW_IF_FAIL(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

#define THROW_IF_FAIL1(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

namespace {
    // SQLite不支援uint64_t格式但可以使用QByteArray保存.
    uint64_t GetUlonglongValue(const SqlQuery& q, const int32_t index) {
        auto blob_size_t = q.value(index).toByteArray();
        XAMP_ASSERT(blob_size_t.size() == sizeof(uint64_t));
        uint64_t result = 0;
        MemoryCopy(&result, blob_size_t.data(), blob_size_t.length());
        return result;
    }

    // SQLite不支援uint64_t格式但可以使用QByteArray保存.
    void BindUlonglongValue(SqlQuery& q, const ConstLatin1String placeholder, const uint64_t v) {
        QByteArray blob_size_t;
        blob_size_t.resize(sizeof(uint64_t));
        MemoryCopy(blob_size_t.data(), &v, sizeof(uint64_t));
        q.bindValue(placeholder, blob_size_t);
    }
}

QString DatabaseFactory::GetDatabaseId() {
    return qTEXT("xamp_db_") + QString::number(reinterpret_cast<quint64>(QThread::currentThread()), 16);
}

PooledDatabasePtr GetPooledDatabase(int32_t pool_size) {
    return std::make_shared<ObjectPool<Database, DatabaseFactory>>(pool_size);
}

SqlException::SqlException(QSqlError error)
    : Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
        error.text().toStdString()) {
    XAMP_LOG_DEBUG("SqlException: {}{}", error.text().toStdString(), GetStackTrace());
}

const char* SqlException::what() const noexcept {
    return message_.c_str();
}

XAMP_DECLARE_LOG_NAME(Database);

Database::Database(const QString& name) {
    logger_ = LoggerManager::GetInstance().GetLogger(kDatabaseLoggerName);
    if (QSqlDatabase::contains(name)) {
        db_ = QSqlDatabase::database(name);
    }
    else {
        db_ = QSqlDatabase::addDatabase(qTEXT("QSQLITE"), name);
    }
    connection_name_ = name;
}

Database::Database()
	: Database(qTEXT("UI")) {
}

QSqlDatabase& Database::database() {
    return db_;
}

Database::~Database() {
    close();
}

void Database::close() {
    if (db_.isOpen()) {
        XAMP_LOG_I(logger_, "Database {} closed.", connection_name_.toStdString());
        db_.close();
    }
}

void Database::open() {
    db_.setDatabaseName(qTEXT("xamp.db"));

    if (!db_.open()) {
        throw SqlException(db_.lastError());
    }

    (void)db_.exec(qTEXT("PRAGMA synchronous = OFF"));    
    (void)db_.exec(qTEXT("PRAGMA auto_vacuum = OFF"));
    (void)db_.exec(qTEXT("PRAGMA foreign_keys = ON"));
    (void)db_.exec(qTEXT("PRAGMA journal_mode = DELETE"));
    (void)db_.exec(qTEXT("PRAGMA cache_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA temp_store = MEMORY"));
    (void)db_.exec(qTEXT("PRAGMA mmap_size = 40960"));
    (void)db_.exec(qTEXT("PRAGMA busy_timeout = 1000"));
    //(void)db_.exec(qTEXT("PRAGMA locking_mode = EXCLUSIVE"));

    XAMP_LOG_I(logger_, "Database {} opened, SQlite version: {}.",
        connection_name_.toStdString(), GetVersion().toStdString());

    CreateTableIfNotExist();
}

bool Database::transaction() {
    return db_.transaction();
}

bool Database::commit() {
    return db_.commit();
}

void Database::rollback() {
    db_.rollback();
}

QString Database::GetVersion() const {
    SqlQuery query(db_);
    query.exec(qTEXT("SELECT sqlite_version() AS version;"));
    if (query.next()) {
        return query.value(qTEXT("version")).toString();
    }
    throw SqlException(query.lastError());
}

void Database::CreateTableIfNotExist() {
    QList<QLatin1String> create_table_sql;

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS musics (
                       musicId integer PRIMARY KEY AUTOINCREMENT,
                       track integer,
                       title TEXT,
                       path TEXT NOT NULL,
                       parentPath TEXT NO NULL,
                       offset DOUBLE,
                       duration DOUBLE,
                       durationStr TEXT,
                       fileName TEXT,
                       fileExt TEXT,
                       bitRate integer,
                       sampleRate integer,
                       rating integer,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       albumReplayGain DOUBLE,
                       albumPeak DOUBLE,
                       trackReplayGain DOUBLE,
                       trackPeak DOUBLE,
                       genre TEXT,
                       comment TEXT,
                       fileSize integer,
                       heart integer,
					   lyrc TEXT,
					   trLyrc TEXT,
                       UNIQUE(path, offset)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                      CREATE UNIQUE INDEX IF NOT EXISTS path_index ON musics (path, offset);
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlist (
                       playlistId integer PRIMARY KEY AUTOINCREMENT,
                       playlistIndex integer,
                       name TEXT NOT NULL
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS tables (
                       tableId integer PRIMARY KEY AUTOINCREMENT,
                       tableIndex integer,
                       playlistId integer,
                       name TEXT NOT NULL
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS tablePlaylist (
                       playlistId integer,
                       tableId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(tableId) REFERENCES tables(tableId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS albums (
                       albumId integer PRIMARY KEY AUTOINCREMENT,
                       artistId integer,
                       album TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
                       discId TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       year integer,
                       heart integer,
                       isPodcast integer,
                       genre TEXT,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       UNIQUE(albumId, artistId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS artists (
                       artistId integer PRIMARY KEY AUTOINCREMENT,
                       artist TEXT NOT NULL DEFAULT '',
                       artistNameEn TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
                       firstChar TEXT,
					   firstCharEn TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumArtist (
                       albumArtistId integer primary key autoincrement,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumCategories (
                       albumCategoryId integer primary key autoincrement,
                       category TEXT,
                       albumId integer,
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumMusic (
                       albumMusicId integer PRIMARY KEY AUTOINCREMENT,
                       musicId integer,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId),
                       UNIQUE(musicId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS musicLoudness (
                       musicLoudnessId integer PRIMARY KEY AUTOINCREMENT,
                       musicId integer,
                       artistId integer,
                       albumId integer,
                       trackLoudness DOUBLE,
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId),
                       UNIQUE(musicId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlistMusics (
                       playlistMusicsId integer PRIMARY KEY AUTOINCREMENT,
                       playlistId integer,
                       musicId integer,
                       albumId integer,
                       playing integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

    create_table_sql.push_back(
        qTEXT(R"(
                       CREATE TABLE IF NOT EXISTS pendingPlaylist (
                       pendingPlaylistId integer PRIMARY KEY AUTOINCREMENT,
                       playlistId integer,
                       playlistMusicsId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(playlistMusicsId) REFERENCES playlistMusics(playlistMusicsId)
                       )
                       )"));

    SqlQuery query(db_);
    Q_FOREACH (const auto& sql , create_table_sql) {
        THROW_IF_FAIL(query, sql);
    }
}

bool Database::DropAllTable() {
    SqlQuery dropQuery(db_);

    QString dropQueryString = "DROP TABLE IF EXISTS %1";
    auto tableNames = db_.tables();

    tableNames.removeAll("sqlite_sequence");
    for (const auto& tableName : qAsConst(tableNames)) {
        THROW_IF_FAIL(dropQuery, dropQueryString.arg(tableName));
    }
    return true;
}

void Database::ClearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistMusicsId != :skipPlaylistMusicsId)"));
    query.bindValue(qTEXT(":playing"), PlayingState::PLAY_CLEAR);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":skipPlaylistMusicsId"), skip_playlist_music_id);
    THROW_IF_FAIL1(query);
}

void Database::ClearNowPlaying(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing"));
    query.bindValue(qTEXT(":playing"), PlayingState::PLAY_CLEAR);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);
}

void Database::SetNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing) {
    SqlQuery query(db_);
    query.prepare(qTEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
    query.bindValue(qTEXT(":playing"), playing);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":playlistMusicsId"), playlist_music_id);
    THROW_IF_FAIL1(query);
}

void Database::ClearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
    SetNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_CLEAR);
}

void Database::SetNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
    SetNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_PLAYING);
}

void Database::RemovePlaylistMusics(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM playlistMusics WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    THROW_IF_FAIL1(query);
}

void Database::RemoveAlbumMusicAlbum(int32_t album_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumMusic WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    THROW_IF_FAIL1(query);
}

void Database::RemoveAlbumCategory(int32_t album_id) {
	SqlQuery query(db_);
	query.prepare(qTEXT("DELETE FROM albumCategories WHERE albumId=:albumId"));
	query.bindValue(qTEXT(":albumId"), album_id);
	THROW_IF_FAIL1(query);
}

void Database::RemoveAlbumMusicId(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumMusic WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    THROW_IF_FAIL1(query);
}

void Database::RemoveTrackLoudnessMusicId(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM musicLoudness WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    THROW_IF_FAIL1(query);
}

void Database::RemoveAlbumArtistId(int32_t artist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumArtist WHERE artistId=:artistId"));
    query.bindValue(qTEXT(":artistId"), artist_id);
    THROW_IF_FAIL1(query);
}

void Database::RemoveMusic(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM musics WHERE musicId=:musicId"));
    query.bindValue(qTEXT(":musicId"), music_id);
    THROW_IF_FAIL1(query);
}

std::optional<QString> Database::GetAlbumFirstMusicFilePath(int32_t album_id) {
    SqlQuery query(qTEXT(R"(
SELECT
    musics.path
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = ?
LIMIT
    1;
)"), db_);
    query.addBindValue(album_id);
    query.exec();
    while (query.next()) {
        return query.value(qTEXT("path")).toString();
    }
    return std::nullopt;
}

void Database::RemoveMusic(QString const& file_path) {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT musicId FROM musics WHERE path = (:path)"));
    query.bindValue(qTEXT(":path"), file_path);

    query.exec();

    while (query.next()) {
        auto music_id = query.value(qTEXT("musicId")).toInt();
        RemovePlaylistMusic(1, QVector<int32_t>{ music_id });
        RemoveAlbumMusicId(music_id);
        RemoveTrackLoudnessMusicId(music_id);
        RemoveAlbumArtistId(music_id);
        RemoveMusic(music_id);
        return;
    }
}

void Database::RemoveAlbumArtist(int32_t album_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albumArtist WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    THROW_IF_FAIL1(query);
}

void Database::ForEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun) {
    QSqlTableModel model(nullptr, db_);

    model.setTable(qTEXT("playlist"));
    model.setSort(1, Qt::AscendingOrder);
    model.select();

    for (auto i = 0; i < model.rowCount(); ++i) {
        auto record = model.record(i);
        fun(record.value(qTEXT("playlistId")).toInt(),
            record.value(qTEXT("playlistIndex")).toInt(),
            record.value(qTEXT("name")).toString());
    }
}

void Database::ForEachAlbumCover(std::function<void(QString)>&& fun, int limit) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
SELECT
    albums.coverId
FROM
    albums
LEFT 
	JOIN artists ON artists.artistId = albums.artistId
WHERE 
	albums.isPodcast = 0
LIMIT
    :limit
    )"));
    query.bindValue(qTEXT(":limit"), limit);
    THROW_IF_FAIL1(query);
    while (query.next()) {
        fun(query.value(qTEXT("coverId")).toString());
    }
}

void Database::ForEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun) {
    
    QSqlTableModel model(nullptr, db_);

    model.setTable(qTEXT("tables"));
    model.setSort(1, Qt::AscendingOrder);
    model.select();

    for (auto i = 0; i < model.rowCount(); ++i) {
        auto record = model.record(i);
        fun(record.value(qTEXT("tableId")).toInt(),
            record.value(qTEXT("tableIndex")).toInt(),
            record.value(qTEXT("playlistId")).toInt(),
            record.value(qTEXT("name")).toString());
    }
}

void Database::ForEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
    SqlQuery query(qTEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
    musicLoudness.trackLoudness,
    albums.album,
    albums.coverId,
    artists.artist,
    musics.*,
    albums.discId
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
    LEFT JOIN musicLoudness ON musicLoudness.musicId = albumMusic.musicId
WHERE
    albums.albumId = ?
)"), db_);
    query.addBindValue(album_id);

    if (!query.exec()) {
        XAMP_LOG_D(logger_, "{}", query.lastError().text().toStdString());
    }

    while (query.next()) {
        fun(FromSqlQuery(query));
    }
}

void Database::RemoveAllArtist() {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM artists"));
    THROW_IF_FAIL1(query);
}

void Database::RemoveArtistId(int32_t artist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM artists WHERE artistId=:artistId"));
    query.bindValue(qTEXT(":artistId"), artist_id);
    THROW_IF_FAIL1(query);
}

void Database::ForEachAlbum(std::function<void(int32_t)>&& fun) {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT albumId FROM albums"));
    THROW_IF_FAIL1(query);
    while (query.next()) {
        fun(query.value(qTEXT("albumId")).toInt());
    }
}

void Database::RemoveAlbum(int32_t album_id) {
    QList<PlayListEntity> entities;
    ForEachAlbumMusic(album_id, [this, &entities](auto const& entity) {
        entities.push_back(entity);        
    });
    Q_FOREACH(const auto & entity, entities) {
        QList<int32_t> playlist_ids;
        ForEachPlaylist([&playlist_ids, this](auto playlistId, auto, auto) {
            playlist_ids.push_back(playlistId);            
        });
        Q_FOREACH(auto playlistId, playlist_ids) {
            RemovePendingListMusic(playlistId);
            RemovePlaylistMusic(playlistId, QVector<int32_t>{ entity.music_id });
        }
        RemoveAlbumCategory(album_id);
        RemoveAlbumMusicAlbum(album_id);
        RemoveTrackLoudnessMusicId(entity.music_id);
        RemoveMusic(entity.music_id);
    }
    RemoveAlbumArtist(album_id);
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM albums WHERE albumId=:albumId"));
    query.bindValue(qTEXT(":albumId"), album_id);
    THROW_IF_FAIL1(query);
}

QStringList Database::GetYears() const {
    QStringList years;

    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
SELECT
    year AS group_year,
    COUNT(*) AS count
FROM albums
WHERE group_year != 0
GROUP BY group_year
ORDER BY group_year DESC;
    )")
    );

    THROW_IF_FAIL1(query);

    while (query.next()) {
        auto genre = query.value(qTEXT("group_year")).toString();
        if (genre.isEmpty()) {
            continue;
        }
        years.append(genre);
    }
    return years;
}

QStringList Database::GetGenres() const {    
    QStringList genres;

    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
SELECT
    CASE
        WHEN instr(albums.genre, ',') > 0 THEN substr(albums.genre, 1, instr(albums.genre, ',') - 1)
        ELSE albums.genre
    END AS group_name,
    COUNT(*) AS count
FROM albums
GROUP BY group_name
ORDER BY count DESC;
    )")
    );

    THROW_IF_FAIL1(query);

    while (query.next()) {
        auto genre = query.value(qTEXT("group_name")).toString();
        if (genre.isEmpty()) {
            continue;
        }
        genres.append(genre);
    }
    return genres;
}

int32_t Database::AddTable(const QString& name, int32_t table_index, int32_t playlist_id) {
    QSqlTableModel model(nullptr, db_);

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(qTEXT("tables"));
    model.select();

    if (!model.insertRows(0, 1)) {
        return kInvalidDatabaseId;
    }

    model.setData(model.index(0, 0), QVariant());
    model.setData(model.index(0, 1), table_index);
    model.setData(model.index(0, 2), playlist_id);
    model.setData(model.index(0, 3), name);

    if (!model.submitAll()) {
        return kInvalidDatabaseId;
    }

    model.database().commit();
    return model.query().lastInsertId().toInt();
}

int32_t Database::AddPlaylist(const QString& name, int32_t playlist_index) {
    QSqlTableModel model(nullptr, db_);

    model.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model.setTable(qTEXT("playlist"));
    model.select();

    if (!model.insertRows(0, 1)) {
        return kInvalidDatabaseId;
    }

    model.setData(model.index(0, 0), QVariant());
    model.setData(model.index(0, 1), playlist_index);
    model.setData(model.index(0, 2), name);

    if (!model.submitAll()) {
        return kInvalidDatabaseId;
    }

    model.database().commit();
    return model.query().lastInsertId().toInt();
}

void Database::SetTableName(int32_t table_id, const QString& name) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE tables SET name = :name WHERE (tableId = :tableId)"));

    query.bindValue(qTEXT(":tableId"), table_id);
    query.bindValue(qTEXT(":name"), name);
    THROW_IF_FAIL1(query);
}

void Database::SetAlbumCover(int32_t album_id, const QString& cover_id) {
    XAMP_ENSURES(!cover_id.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":coverId"), cover_id);

    THROW_IF_FAIL1(query);
    XAMP_LOG_D(logger_, "setAlbumCover albumId: {} coverId: {}", album_id, cover_id.toStdString());
}

void Database::SetAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
    XAMP_ENSURES(!cover_id.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) AND (album = :album)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":album"), album);
    query.bindValue(qTEXT(":coverId"), cover_id);

    THROW_IF_FAIL1(query);
    XAMP_LOG_D(logger_, "setAlbumCover albumId: {} coverId: {}", album_id, cover_id.toStdString());
}

std::optional<ArtistStats> Database::GetArtistStats(int32_t artist_id) const {    
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        (SELECT COUNT( * ) AS albums FROM albums WHERE albums.artistId = :artistId) AS albums,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.artistId = :artistId) AS tracks
    FROM
        albumMusic
    JOIN albums ON albums.artistId = albumMusic.artistId
    JOIN musics ON musics.musicId = albumMusic.musicId
    WHERE
        albums.artistId = :artistId;)"));

    query.bindValue(qTEXT(":artistId"), artist_id);

    THROW_IF_FAIL1(query);

    while (query.next()) {
        ArtistStats stats;
        stats.albums = query.value(qTEXT("albums")).toInt();
        stats.tracks = query.value(qTEXT("tracks")).toInt();
        stats.durations = query.value(qTEXT("durations")).toDouble();
        return stats;
    }

    return std::nullopt;
}

std::optional<AlbumStats> Database::GetAlbumStats(int32_t album_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        MAX(year) AS year,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.albumId = :albumId) AS tracks,
        SUM(musics.fileSize) AS fileSize
    FROM
        albumMusic
    JOIN albums ON albums.albumId = albumMusic.albumId
    JOIN musics ON musics.musicId = albumMusic.musicId
    WHERE
        albums.albumId = :albumId;)"));

    query.bindValue(qTEXT(":albumId"), album_id);

    THROW_IF_FAIL1(query);

    while (query.next()) {
        AlbumStats stats;
        stats.songs = query.value(qTEXT("tracks")).toInt();
        stats.year = query.value(qTEXT("year")).toInt();
        stats.durations = query.value(qTEXT("durations")).toDouble();
        stats.file_size = query.value(qTEXT("fileSize")).toULongLong();
        return stats;
    }

    return std::nullopt;
}

bool Database::IsPlaylistExist(int32_t playlist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);

    THROW_IF_FAIL1(query);
    return query.next();
}

QStringList Database::GetCategories() const {
    QStringList categories;
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT category FROM albumCategories GROUP BY category"));

    THROW_IF_FAIL1(query);

    while (query.next()) {
        categories.append(query.value(qTEXT("category")).toString());
    }

    return categories;
}

int32_t Database::FindTablePlaylistId(int32_t table_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT playlistId FROM tablePlaylist WHERE tableId = (:tableId)"));
    query.bindValue(qTEXT(":tableId"), table_id);

    THROW_IF_FAIL1(query);
    while (query.next()) {
        return query.value(qTEXT("playlistId")).toInt();
    }
    return kInvalidDatabaseId;
}

void Database::AddTablePlaylist(int32_t table_id, int32_t playlist_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
    )"));

    query.bindValue(qTEXT(":playlistId"), playlist_id);
    query.bindValue(qTEXT(":tableId"), table_id);

    THROW_IF_FAIL1(query);
}

QString Database::GetArtistCoverId(int32_t artist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
    query.bindValue(qTEXT(":artistId"), artist_id);

    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

QString Database::GetAlbumCoverId(int32_t album_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
    query.bindValue(qTEXT(":albumId"), album_id);

    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

int32_t Database::GetAlbumId(const QString& album) const {
	SqlQuery query(db_);

	query.prepare(qTEXT("SELECT albumId FROM albums WHERE album = (:album)"));
	query.bindValue(qTEXT(":album"), album);

	THROW_IF_FAIL1(query);
	const auto index = query.record().indexOf(qTEXT("albumId"));
    if (query.next()) {
		return query.value(index).toInt();
	}
	return kInvalidDatabaseId;
}

QString Database::GetAlbumCoverId(const QString& album) const {
    SqlQuery query(db_);

    query.prepare(qTEXT("SELECT coverId FROM albums WHERE album = (:album)"));
    query.bindValue(qTEXT(":album"), album);

    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("coverId"));
    if (query.next()) {
        return query.value(index).toString();
    }
    return kEmptyString;
}

PlayListEntity Database::FromSqlQuery(const SqlQuery& query) {
    PlayListEntity entity;
    entity.album_id = query.value(qTEXT("albumId")).toInt();
    entity.artist_id = query.value(qTEXT("artistId")).toInt();
    entity.music_id = query.value(qTEXT("musicId")).toInt();
    entity.file_path = query.value(qTEXT("path")).toString();
    entity.track = query.value(qTEXT("track")).toUInt();
    entity.title = query.value(qTEXT("title")).toString();
    entity.file_name = query.value(qTEXT("fileName")).toString();
    entity.album = query.value(qTEXT("album")).toString();
    entity.artist = query.value(qTEXT("artist")).toString();
    entity.file_extension = query.value(qTEXT("fileExt")).toString();
    entity.parent_path = query.value(qTEXT("parentPath")).toString();
    entity.duration = query.value(qTEXT("duration")).toDouble();
    entity.bit_rate = query.value(qTEXT("bitRate")).toUInt();
    entity.sample_rate = query.value(qTEXT("sampleRate")).toUInt();
    entity.cover_id = query.value(qTEXT("coverId")).toString();
    entity.rating = query.value(qTEXT("rating")).toUInt();
    entity.album_replay_gain = query.value(qTEXT("albumReplayGain")).toDouble();
    entity.album_peak = query.value(qTEXT("albumPeak")).toDouble();
    entity.track_replay_gain = query.value(qTEXT("trackReplayGain")).toDouble();
    entity.track_peak = query.value(qTEXT("trackPeak")).toDouble();
    entity.track_loudness = query.value(qTEXT("trackLoudness")).toDouble();

    entity.genre = query.value(qTEXT("genre")).toString();
    entity.comment = query.value(qTEXT("comment")).toString();
    entity.file_size = query.value(qTEXT("fileSize")).toULongLong();

    entity.lyrc = query.value(qTEXT("lyrc")).toString();
    entity.trlyrc = query.value(qTEXT("trLyrc")).toString();

    return entity;
}

std::optional<std::tuple<QString, QString>> Database::GetLyrc(int32_t music_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
    SELECT
        lyrc, trLyrc
    FROM
        musics
    WHERE
        musicId = :musicId
    )")
    );

    query.bindValue(qTEXT(":musicId"), music_id);

    THROW_IF_FAIL1(query);

    while (query.next()) {
        auto lyrc = query.value(qTEXT("lyrc")).toString();
        auto tr_lyrc = query.value(qTEXT("trLyrc")).toString();
        if (!lyrc.isEmpty()) {
            return std::tuple<QString, QString> { lyrc, tr_lyrc };
        }
    }
    return std::nullopt;
}

int32_t Database::AddOrUpdateMusic(const TrackInfo& track_info) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitRate, sampleRate, offset, dateTime, albumReplayGain, trackReplayGain, albumPeak, trackPeak, genre, comment, fileSize)
    VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitRate, :sampleRate, :offset, :dateTime, :albumReplayGain, :trackReplayGain, :albumPeak, :trackPeak, :genre, :comment, :fileSize)
    )")
    );

    query.bindValue(qTEXT(":title"), GetStringOrEmptyString(track_info.title));
    query.bindValue(qTEXT(":track"), track_info.track);
    query.bindValue(qTEXT(":path"), GetStringOrEmptyString(track_info.file_path));
    query.bindValue(qTEXT(":fileExt"), GetStringOrEmptyString(track_info.file_ext));
    query.bindValue(qTEXT(":fileName"), GetStringOrEmptyString(track_info.file_name));
    query.bindValue(qTEXT(":parentPath"), GetStringOrEmptyString(track_info.parent_path));
    query.bindValue(qTEXT(":duration"), track_info.duration);
    query.bindValue(qTEXT(":durationStr"), FormatDuration(track_info.duration));
    query.bindValue(qTEXT(":bitRate"), track_info.bit_rate);
    query.bindValue(qTEXT(":sampleRate"), track_info.sample_rate);
    query.bindValue(qTEXT(":offset"), track_info.offset);
    query.bindValue(qTEXT(":fileSize"), track_info.file_size);

    if (track_info.replay_gain) {
        query.bindValue(qTEXT(":albumReplayGain"), track_info.replay_gain.value().album_gain);
        query.bindValue(qTEXT(":trackReplayGain"), track_info.replay_gain.value().track_gain);
        query.bindValue(qTEXT(":albumPeak"), track_info.replay_gain.value().album_peak);
        query.bindValue(qTEXT(":trackPeak"), track_info.replay_gain.value().track_peak);
    }
    else {
        query.bindValue(qTEXT(":albumReplayGain"), 0);
        query.bindValue(qTEXT(":trackReplayGain"), 0);
        query.bindValue(qTEXT(":albumPeak"), 0);
        query.bindValue(qTEXT(":trackPeak"), 0);
    }

    query.bindValue(qTEXT(":dateTime"), track_info.last_write_time);
    query.bindValue(qTEXT(":genre"), GetStringOrEmptyString(track_info.genre));
    query.bindValue(qTEXT(":comment"), GetStringOrEmptyString(track_info.comment));

    THROW_IF_FAIL1(query);

    const auto music_id = query.lastInsertId().toInt();

    XAMP_LOG_D(logger_, "addOrUpdateMusic musicId:{}", music_id);

    XAMP_ENSURES(music_id != kInvalidDatabaseId);
    return music_id;
}

void Database::AddOrUpdateLyrc(int32_t music_id, const QString& lyrc, const QString& trlyrc) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET lyrc = :lyrc, trlyrc = :trlyrc WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":lyrc"), lyrc);
    query.bindValue(qTEXT(":trlyrc"), trlyrc);

    query.exec();
}

void Database::UpdateMusicFilePath(int32_t music_id, const QString& file_path) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":path"), file_path);

    query.exec();
}

void Database::UpdateMusicRating(int32_t music_id, int32_t rating) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":rating"), rating);

    THROW_IF_FAIL1(query);
}

void Database::UpdateAlbumHeart(int32_t album_id, uint32_t heart) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET heart = :heart WHERE (albumId = :albumId)"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":heart"), heart);

    THROW_IF_FAIL1(query);
}

void Database::UpdateMusicHeart(int32_t music_id, uint32_t heart) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET heart = :heart WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":heart"), heart);

    THROW_IF_FAIL1(query);
}

void Database::UpdateMusicTitle(int32_t music_id, const QString& title) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET title = :title WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":title"), title);

    THROW_IF_FAIL1(query);
}

void Database::AddOrUpdateTrackLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musicLoudness (musicLoudnessId, albumId, artistId, musicId, trackLoudness)
    VALUES
    (
    (SELECT musicLoudnessId FROM musicLoudness WHERE albumId = :albumId AND artistId = :artistId AND musicId = :musicId AND trackLoudness = :trackLoudness),
    :albumId,
    :artistId,
    :musicId,
    :trackLoudness
    )
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":trackLoudness"), track_loudness);

    THROW_IF_FAIL1(query);
}

void Database::UpdateReplayGain(int32_t music_id,
    double album_rg_gain,
    double album_peak,
    double track_rg_gain,
    double track_peak) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE musics SET albumReplayGain = :albumReplayGain, albumPeak = :albumPeak, trackReplayGain = :trackReplayGain, trackPeak = :trackPeak WHERE (musicId = :musicId)"));

    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":albumReplayGain"), album_rg_gain);
    query.bindValue(qTEXT(":albumPeak"), album_peak);
    query.bindValue(qTEXT(":trackReplayGain"), track_rg_gain);
    query.bindValue(qTEXT(":trackPeak"), track_peak);

    THROW_IF_FAIL1(query);
}

void Database::AddMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const {
    SqlQuery query(db_);

    const auto querystr = qSTR("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId) VALUES (NULL, %1, %2, %3)")
        .arg(playlist_id)
        .arg(music_id)
        .arg(album_id);

    query.prepare(querystr);
    THROW_IF_FAIL1(query);
}

void Database::AddMusicToPlaylist(const QList<int32_t>& music_id, int32_t playlist_id) const {
    SqlQuery query(db_);

    QStringList strings;

    for (const auto id : music_id) {
        strings << qTEXT("(") + qTEXT("NULL, ") + QString::number(playlist_id) + qTEXT(", ") + QString::number(id) + qTEXT(")");
    }

    const auto querystr = qTEXT("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES ")
    + strings.join(qTEXT(","));
    query.prepare(querystr);
    THROW_IF_FAIL1(query);
}

void Database::UpdateArtistEnglishName(const QString& artist, const QString& en_name) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE artists SET artistNameEn = :artistNameEn, firstCharEn = :firstCharEn WHERE (artist = :artist AND firstCharEn IS NULL)"));

    query.bindValue(qTEXT(":artist"), artist);
    query.bindValue(qTEXT(":artistNameEn"), en_name);
    query.bindValue(qTEXT(":firstCharEn"), en_name.left(1));

    THROW_IF_FAIL1(query);
}

void Database::UpdateArtistCoverId(int32_t artist_id, const QString& cover_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":coverId"), cover_id);

    THROW_IF_FAIL1(query);
}

int32_t Database::AddOrUpdateArtist(const QString& artist) {
    XAMP_EXPECTS(!artist.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"));

    const auto first_char = artist.left(1);
    query.bindValue(qTEXT(":artist"), artist);
    query.bindValue(qTEXT(":firstChar"), first_char.toUpper());

    THROW_IF_FAIL1(query);

    const auto artist_id = query.lastInsertId().toInt();
    return artist_id;
}

void Database::UpdateArtistByDiscId(const QString& disc_id, const QString& artist) {
    XAMP_EXPECTS(!artist.isEmpty());

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    SELECT
        albumMusic.artistId
    FROM
        albumMusic
    JOIN albums ON albums.albumId = albumMusic.albumId
    WHERE
        albums.discId = :discId
    LIMIT 1
    )"));

    query.bindValue(qTEXT(":discId"), disc_id);
    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("artistId"));
    if (query.next()) {
        auto artist_id = query.value(index).toInt();
        query.prepare(qTEXT("UPDATE artists SET artist = :artist WHERE (artistId = :artistId)"));
        query.bindValue(qTEXT(":artistId"), artist_id);
        query.bindValue(qTEXT(":artist"), artist);
        THROW_IF_FAIL1(query);
    }
}

int32_t Database::GetAlbumIdByDiscId(const QString& disc_id) const {
    SqlQuery query(db_);
    query.prepare(qTEXT("SELECT albumId FROM albums WHERE discId = (:discId)"));
    query.bindValue(qTEXT(":discId"), disc_id);
    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("albumId"));
    if (query.next()) {
        return query.value(index).toInt();
    }
    return kInvalidDatabaseId;
}

void Database::UpdateAlbumByDiscId(const QString& disc_id, const QString& album) {
    SqlQuery query(db_);

    query.prepare(qTEXT("UPDATE albums SET album = :album WHERE (discId = :discId)"));

    query.bindValue(qTEXT(":album"), album);
    query.bindValue(qTEXT(":discId"), disc_id);

    THROW_IF_FAIL1(query);
}

int32_t Database::AddOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, uint32_t year, bool is_podcast, const QString& disc_id, const QString & album_genre) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO albums (albumId, album, artistId, coverId, isPodcast, dateTime, discId, year, genre)
    VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId, :coverId, :isPodcast, :dateTime, :discId, :year, :genre)
    )"));

    query.bindValue(qTEXT(":album"), album);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":coverId"), GetAlbumCoverId(album));
    query.bindValue(qTEXT(":isPodcast"), is_podcast ? 1 : 0);
    query.bindValue(qTEXT(":dateTime"), album_time);
    query.bindValue(qTEXT(":discId"), disc_id);
    query.bindValue(qTEXT(":year"), year);
    query.bindValue(qTEXT(":genre"), album_genre);

    THROW_IF_FAIL1(query);

    const auto album_id = query.lastInsertId().toInt();

    XAMP_LOG_D(logger_, "addOrUpdateAlbum albumId:{}", album_id);
    return album_id;
}

std::pair<int32_t, int32_t> Database::GetFirstPendingPlaylistMusic(int32_t playlist_id) {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
SELECT
    pendingPlaylistId,
	musics.musicId 
FROM
	pendingPlaylist 
JOIN playlistMusics ON pendingPlaylist.playlistMusicsId = playlistMusics.playlistMusicsId
JOIN musics ON playlistMusics.musicId = musics.musicId
WHERE
	playlistMusics.playlistId = :playlistId
LIMIT 1
    )"));

    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);

    const auto index = query.record().indexOf(qTEXT("musicId"));
    if (!query.next()) {
        return std::make_pair(kInvalidDatabaseId, kInvalidDatabaseId);
    }

    return std::make_pair(
        query.value(qTEXT("musicId")).toInt(),
        query.value(qTEXT("pendingPlaylistId")).toInt());
}

void Database::ClearPendingPlaylist() {
    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
DELETE
FROM
    pendingPlaylist
    )"));
    THROW_IF_FAIL1(query);
}

void Database::ClearPendingPlaylist(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
DELETE
FROM
    pendingPlaylist
WHERE
    playlistId = :playlistId
    )"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);
}

void Database::DeletePendingPlaylistMusic(int32_t pending_playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT(R"(
DELETE 
FROM
	pendingPlaylist 
WHERE
	pendingPlaylistId = :pendingPlaylistId
    )"));
    query.bindValue(qTEXT(":pendingPlaylistId"), pending_playlist_id);
    THROW_IF_FAIL1(query);
}

void Database::AddPendingPlaylist(int32_t playlist_musics_id, int32_t playlist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO pendingPlaylist (pendingPlaylistId, playlistMusicsId, playlistId)
    VALUES (NULL, :playlistMusicsId, :playlistId)
    )"));
    
    query.bindValue(qTEXT(":playlistMusicsId"), playlist_musics_id);
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);
}

void Database::AddOrUpdateAlbumCategory(int32_t album_id, const QString& category) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO albumCategories (albumCategoryId, albumId, category)
    VALUES (NULL, :albumId, :category)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":category"), category);

    THROW_IF_FAIL1(query);
}

void Database::AddOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES (NULL, :albumId, :artistId)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);

    THROW_IF_FAIL1(query);

    XAMP_LOG_D(logger_, "addOrUpdateAlbumArtist albumId:{} artistId:{}", album_id, artist_id);
}

void Database::AddOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) const {
    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musicLoudness (albumMusicId, albumId, artistId, musicId, trackLoudness)
    VALUES ((SELECT albumMusicId from musicLoudness where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId, :trackLoudness)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);
    query.bindValue(qTEXT(":trackLoudness"), track_loudness);

    THROW_IF_FAIL1(query);
}

void Database::AddOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
    XAMP_EXPECTS(album_id != kInvalidDatabaseId);
    XAMP_EXPECTS(artist_id != kInvalidDatabaseId);
    XAMP_EXPECTS(music_id != kInvalidDatabaseId);

    SqlQuery query(db_);

    query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
    VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
    )"));

    query.bindValue(qTEXT(":albumId"), album_id);
    query.bindValue(qTEXT(":artistId"), artist_id);
    query.bindValue(qTEXT(":musicId"), music_id);

    THROW_IF_FAIL1(query);    
}

void Database::RemovePlaylistAllMusic(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);
    XAMP_LOG_D(logger_, "removePlaylistAllMusic playlist_id:{}", playlist_id);
}

void Database::RemovePendingListMusic(int32_t playlist_id) {
    SqlQuery query(db_);
    query.prepare(qTEXT("DELETE FROM pendingPlaylist WHERE playlistId=:playlistId"));
    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);
}

void Database::RemovePlaylistMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
    SqlQuery query(db_);

    QString str = qTEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)");

    QStringList list;
    for (auto id : select_music_ids) {
        list << QString::number(id);
    }

    auto q = str.arg(list.join(qTEXT(",")));
    query.prepare(q);

    query.bindValue(qTEXT(":playlistId"), playlist_id);
    THROW_IF_FAIL1(query);

    if (!query.next()) {
        XAMP_LOG_D(logger_, "RemovePlaylistMusic playlist:{} affected rows: 0 {}", playlist_id, q.toStdString());
    }
}

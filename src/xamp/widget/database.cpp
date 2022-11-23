#include <QSqlTableModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QDateTime>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/assert.h>
#include <widget/str_utilts.h>
#include <widget/database.h>

#define IfFailureThrow(query, sql) \
    do {\
    if (!query.exec(sql)) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

#define IfFailureThrow1(query) \
    do {\
    if (!query.exec()) {\
    throw SqlException(query.lastError());\
    }\
    } while (false)

SqlException::SqlException(QSqlError error)
	: Exception(Errors::XAMP_ERROR_PLATFORM_SPEC_ERROR,
		error.text().toStdString()) {
	XAMP_LOG_DEBUG("SqlException: {}", error.text().toStdString());
}

const char* SqlException::what() const noexcept {
	return message_.c_str();
}

Database::Database() {
	logger_ = LoggerManager::GetInstance().GetLogger("Database");
	db_ = QSqlDatabase::addDatabase(Q_TEXT("QSQLITE"));
}

Database::~Database() {
	db_.close();
}

void Database::open(const QString& file_name) {
	db_.setDatabaseName(file_name);

	if (!db_.open()) {
		throw SqlException(db_.lastError());
	}

	dbname_ = file_name;
	(void)db_.exec(Q_TEXT("PRAGMA synchronous = OFF"));
	(void)db_.exec(Q_TEXT("PRAGMA auto_vacuum = OFF"));
	(void)db_.exec(Q_TEXT("PRAGMA foreign_keys = ON"));
	(void)db_.exec(Q_TEXT("PRAGMA journal_mode = MEMORY"));
	(void)db_.exec(Q_TEXT("PRAGMA cache_size = 40960"));
	(void)db_.exec(Q_TEXT("PRAGMA temp_store = MEMORY"));
	(void)db_.exec(Q_TEXT("PRAGMA mmap_size = 40960"));

	XAMP_LOG_I(logger_, "SQlite version: {}", getVersion().toStdString());

	createTableIfNotExist();
}

void Database::transaction() {
	db_.transaction();
}

void Database::commit() {
	db_.commit();
}

void Database::rollback() {
	db_.rollback();
}

QString Database::getVersion() const {
	QSqlQuery query;
	query.exec(Q_TEXT("SELECT sqlite_version() AS version;"));
	if (query.next()) {
		return query.value(Q_TEXT("version")).toString();
	}
	throw SqlException(query.lastError());
}

void Database::createTableIfNotExist() {
	std::vector<QLatin1String> create_table_sql;

	create_table_sql.push_back(
		Q_TEXT(R"(
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
                       bitrate integer,
                       samplerate integer,
					   rating integer,					
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                       album_replay_gain DOUBLE,
                       album_peak DOUBLE,
                       track_replay_gain DOUBLE,
                       track_peak DOUBLE,
					   genre TEXT,
					   comment TEXT,
					   year integer,
					   fileSize integer,
					   parentPathHash integer,
                       UNIQUE(path, offset)
					   )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                      CREATE UNIQUE INDEX IF NOT EXISTS path_index ON musics (path, offset);
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlist (
                       playlistId integer PRIMARY KEY AUTOINCREMENT,
                       playlistIndex integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS tables (
                       tableId integer PRIMARY KEY AUTOINCREMENT,
                       tableIndex integer,
                       playlistId integer,
                       name TEXT NOT NULL
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS tablePlaylist (
                       playlistId integer,
                       tableId integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(tableId) REFERENCES tables(tableId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albums (
                       albumId integer primary key autoincrement,
                       artistId integer,
                       album TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
					   discId TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
					   isPodcast integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
					   UNIQUE(albumId, artistId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS artists (
                       artistId integer primary key autoincrement,
                       artist TEXT NOT NULL DEFAULT '',
                       coverId TEXT,
					   firstChar TEXT,
                       dateTime TIMESTAMP DEFAULT CURRENT_TIMESTAMP
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumArtist (
                       albumArtistId integer primary key autoincrement,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS albumMusic (
                       albumMusicId integer primary key autoincrement,
                       musicId integer,
                       artistId integer,
                       albumId integer,
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
                       FOREIGN KEY(artistId) REFERENCES artists(artistId),
                       FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	create_table_sql.push_back(
		Q_TEXT(R"(
                       CREATE TABLE IF NOT EXISTS playlistMusics (
					   playlistMusicsId integer primary key autoincrement,
                       playlistId integer,
                       musicId integer,
					   albumId integer,
                       playing integer,
                       FOREIGN KEY(playlistId) REFERENCES playlist(playlistId),
                       FOREIGN KEY(musicId) REFERENCES musics(musicId),
					   FOREIGN KEY(albumId) REFERENCES albums(albumId)
                       )
                       )"));

	QSqlQuery query(db_);
	for (const auto& sql : create_table_sql) {
		IfFailureThrow(query, sql);
	}
}

void Database::clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistMusicsId != :skipPlaylistMusicsId)"));
	query.bindValue(Q_TEXT(":playing"), PlayingState::PLAY_CLEAR);
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":skipPlaylistMusicsId"), skip_playlist_music_id);
	IfFailureThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = :playing"));
	query.bindValue(Q_TEXT(":playing"), PlayingState::PLAY_CLEAR);
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing) {
	QSqlQuery query;
	query.prepare(Q_TEXT("UPDATE playlistMusics SET playing = :playing WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"));
	query.bindValue(Q_TEXT(":playing"), playing);
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":playlistMusicsId"), playlist_music_id);
	IfFailureThrow1(query);
}

void Database::clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
	setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_CLEAR);
}

void Database::setNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
	setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_PLAYING);
}

void Database::removePlaylistMusics(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM playlistMusics WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeAlbumMusicId(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumMusic WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeAlbumArtistId(int32_t artist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumArtist WHERE artistId=:artistId"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	IfFailureThrow1(query);
}

void Database::removeMusic(int32_t music_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM musics WHERE musicId=:musicId"));
	query.bindValue(Q_TEXT(":musicId"), music_id);
	IfFailureThrow1(query);
}

void Database::removeMusic(QString const& file_path) {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT musicId FROM musics WHERE path = (:path)"));
	query.bindValue(Q_TEXT(":path"), file_path);

	query.exec();

	while (query.next()) {
		auto music_id = query.value(Q_TEXT("musicId")).toInt();
		removePlaylistMusic(1, QVector<int32_t>{ music_id });
		removeAlbumMusicId(music_id);
		removeAlbumArtistId(music_id);
		removeMusic(music_id);
		return;
	}
}

void Database::removeAlbumArtist(int32_t album_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albumArtist WHERE albumId=:albumId"));
	query.bindValue(Q_TEXT(":albumId"), album_id);
	IfFailureThrow1(query);
}

void Database::forEachPlaylist(std::function<void(int32_t, int32_t, QString)>&& fun) {
    QSqlTableModel model(nullptr, db_);

    model.setTable(Q_TEXT("playlist"));
    model.setSort(1, Qt::AscendingOrder);
    model.select();

    for (auto i = 0; i < model.rowCount(); ++i) {
        auto record = model.record(i);
        fun(record.value(Q_TEXT("playlistId")).toInt(),
            record.value(Q_TEXT("playlistIndex")).toInt(),
            record.value(Q_TEXT("name")).toString());
    }
}

void Database::forEachTable(std::function<void(int32_t, int32_t, int32_t, QString)>&& fun) {
	QSqlTableModel model(nullptr, db_);

	model.setTable(Q_TEXT("tables"));
	model.setSort(1, Qt::AscendingOrder);
	model.select();

	for (auto i = 0; i < model.rowCount(); ++i) {
		auto record = model.record(i);
		fun(record.value(Q_TEXT("tableId")).toInt(),
			record.value(Q_TEXT("tableIndex")).toInt(),
			record.value(Q_TEXT("playlistId")).toInt(),
			record.value(Q_TEXT("name")).toString());
	}
}

void Database::forEachAlbumArtistMusic(int32_t album_id, int32_t artist_id, std::function<void(PlayListEntity const&)>&& fun) {
	QSqlQuery query(Q_TEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
    albums.album,
    albums.coverId,
    artists.artist,
    musics.*
FROM
    albumMusic
    LEFT JOIN albums ON albums.albumId = albumMusic.albumId
    LEFT JOIN artists ON artists.artistId = albumMusic.artistId
    LEFT JOIN musics ON musics.musicId = albumMusic.musicId
WHERE
    albums.albumId = ? AND albumMusic.artistId = ?
ORDER BY musics.path;)"));
	query.addBindValue(album_id);
	query.addBindValue(artist_id);

	if (!query.exec()) {
		XAMP_LOG_DEBUG("{}", query.lastError().text().toStdString());
	}

	while (query.next()) {
		PlayListEntity entity;
		entity.album_id = query.value(Q_TEXT("albumId")).toInt();
		entity.artist_id = query.value(Q_TEXT("artistId")).toInt();
		entity.music_id = query.value(Q_TEXT("musicId")).toInt();
		entity.file_path = query.value(Q_TEXT("path")).toString();
		entity.track = query.value(Q_TEXT("track")).toUInt();
		entity.title = query.value(Q_TEXT("title")).toString();
		entity.file_name = query.value(Q_TEXT("fileName")).toString();
		entity.album = query.value(Q_TEXT("album")).toString();
		entity.artist = query.value(Q_TEXT("artist")).toString();
		entity.file_ext = query.value(Q_TEXT("fileExt")).toString();
		entity.parent_path = query.value(Q_TEXT("parentPath")).toString();
		entity.duration = query.value(Q_TEXT("duration")).toDouble();
		entity.bitrate = query.value(Q_TEXT("bitrate")).toUInt();
		entity.samplerate = query.value(Q_TEXT("samplerate")).toUInt();
		entity.cover_id = query.value(Q_TEXT("coverId")).toString();
		entity.rating = query.value(Q_TEXT("rating")).toUInt();
		entity.album_replay_gain = query.value(Q_TEXT("album_replay_gain")).toDouble();
		entity.album_peak = query.value(Q_TEXT("album_peak")).toDouble();
		entity.track_replay_gain = query.value(Q_TEXT("track_replay_gain")).toDouble();
		entity.track_peak = query.value(Q_TEXT("track_peak")).toDouble();

		entity.genre = query.value(Q_TEXT("genre")).toString();
		entity.comment = query.value(Q_TEXT("comment")).toString();
		entity.year = query.value(Q_TEXT("year")).toUInt();
		entity.file_size = query.value(Q_TEXT("fileSize")).toULongLong();
		fun(entity);
	}
}

void Database::forEachAlbumMusic(int32_t album_id, std::function<void(PlayListEntity const&)>&& fun) {
	QSqlQuery query(Q_TEXT(R"(
SELECT
    albumMusic.albumId,
    albumMusic.artistId,
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
WHERE
    albums.albumId = ?
ORDER BY musics.track, musics.path, musics.fileName;)"));
	query.addBindValue(album_id);

	if (!query.exec()) {
		XAMP_LOG_D(logger_, "{}", query.lastError().text().toStdString());
	}

	while (query.next()) {
		PlayListEntity entity;
		entity.album_id = query.value(Q_TEXT("albumId")).toInt();
		entity.artist_id = query.value(Q_TEXT("artistId")).toInt();
		entity.music_id = query.value(Q_TEXT("musicId")).toInt();
		entity.file_path = query.value(Q_TEXT("path")).toString();
		entity.track = query.value(Q_TEXT("track")).toUInt();
		entity.title = query.value(Q_TEXT("title")).toString();
		entity.file_name = query.value(Q_TEXT("fileName")).toString();
		entity.album = query.value(Q_TEXT("album")).toString();
		entity.artist = query.value(Q_TEXT("artist")).toString();
		entity.file_ext = query.value(Q_TEXT("fileExt")).toString();
		entity.parent_path = query.value(Q_TEXT("parentPath")).toString();
		entity.duration = query.value(Q_TEXT("duration")).toDouble();
		entity.bitrate = query.value(Q_TEXT("bitrate")).toUInt();
		entity.samplerate = query.value(Q_TEXT("samplerate")).toUInt();
		entity.cover_id = query.value(Q_TEXT("coverId")).toString();
		entity.rating = query.value(Q_TEXT("rating")).toUInt();
		entity.album_replay_gain = query.value(Q_TEXT("album_replay_gain")).toDouble();
		entity.album_peak = query.value(Q_TEXT("album_peak")).toDouble();
		entity.track_replay_gain = query.value(Q_TEXT("track_replay_gain")).toDouble();
		entity.track_peak = query.value(Q_TEXT("track_peak")).toDouble();
		entity.disc_id = query.value(Q_TEXT("discId")).toString();
		entity.genre = query.value(Q_TEXT("genre")).toString();
		entity.comment = query.value(Q_TEXT("comment")).toString();
		entity.year = query.value(Q_TEXT("year")).toUInt();
		fun(entity);
	}
}

void Database::removeAllArtist() {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM artists"));
	IfFailureThrow1(query);
}

void Database::removeArtistId(int32_t artist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM artists WHERE artistId=:artistId"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	IfFailureThrow1(query);
}

void Database::forEachAlbum(std::function<void(int32_t)>&& fun) {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT albumId FROM albums"));
	IfFailureThrow1(query);
	while (query.next()) {
		fun(query.value(Q_TEXT("albumId")).toInt());
	}
}

void Database::removeAlbum(int32_t album_id) {
	forEachAlbumMusic(album_id, [this](auto const& entity) {
        forEachPlaylist([&entity, this](auto playlistId, auto , auto) {
			removeMusic(playlistId, QVector<int32_t>{ entity.music_id });
        });
		removeAlbumMusicId(entity.music_id);
		removeMusic(entity.music_id);
		});

	removeAlbumArtist(album_id);

	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM albums WHERE albumId=:albumId"));
	query.bindValue(Q_TEXT(":albumId"), album_id);
	IfFailureThrow1(query);
}

int32_t Database::addTable(const QString& name, int32_t table_index, int32_t playlist_id) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable(Q_TEXT("tables"));
	model.select();

	if (!model.insertRows(0, 1)) {
		return kInvalidId;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), table_index);
	model.setData(model.index(0, 2), playlist_id);
	model.setData(model.index(0, 3), name);

	if (!model.submitAll()) {
		return kInvalidId;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

int32_t Database::addPlaylist(const QString& name, int32_t playlist_index) {
	QSqlTableModel model(nullptr, db_);

	model.setEditStrategy(QSqlTableModel::OnManualSubmit);
	model.setTable(Q_TEXT("playlist"));
	model.select();

	if (!model.insertRows(0, 1)) {
		return kInvalidId;
	}

	model.setData(model.index(0, 0), QVariant());
	model.setData(model.index(0, 1), playlist_index);
	model.setData(model.index(0, 2), name);

	if (!model.submitAll()) {
		return kInvalidId;
	}

	model.database().commit();
	return model.query().lastInsertId().toInt();
}

void Database::setTableName(int32_t table_id, const QString& name) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE tables SET name = :name WHERE (tableId = :tableId)"));

	query.bindValue(Q_TEXT(":tableId"), table_id);
	query.bindValue(Q_TEXT(":name"), name);
	IfFailureThrow1(query);
}

void Database::setAlbumCover(int32_t album_id, const QString& cover_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId)"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":coverId"), cover_id);

	IfFailureThrow1(query);
	XAMP_LOG_D(logger_, "setAlbumCover albumId: {} coverId: {}", album_id, cover_id.toStdString());
}

void Database::setAlbumCover(int32_t album_id, const QString& album, const QString& cover_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE albums SET coverId = :coverId WHERE (albumId = :albumId) AND (album = :album)"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":album"), album);
	query.bindValue(Q_TEXT(":coverId"), cover_id);

	IfFailureThrow1(query);
	XAMP_LOG_D(logger_, "setAlbumCover albumId: {} coverId: {}", album_id, cover_id.toStdString());
}

std::optional<ArtistStats> Database::getArtistStats(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
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

	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);

	while (query.next()) {
		ArtistStats stats;
		stats.albums = query.value(Q_TEXT("albums")).toInt();
		stats.tracks = query.value(Q_TEXT("tracks")).toInt();
		stats.durations = query.value(Q_TEXT("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

std::optional<AlbumStats> Database::getAlbumStats(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
		MAX(year) AS year,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.albumId = :albumId) AS tracks
    FROM
	    albumMusic
	JOIN albums ON albums.albumId = albumMusic.albumId 
    JOIN musics ON musics.musicId = albumMusic.musicId 
    WHERE
	    albums.albumId = :albumId;)"));

	query.bindValue(Q_TEXT(":albumId"), album_id);

	IfFailureThrow1(query);

	while (query.next()) {
		AlbumStats stats;
		stats.tracks = query.value(Q_TEXT("tracks")).toInt();
		stats.year = query.value(Q_TEXT("year")).toInt();
		stats.durations = query.value(Q_TEXT("durations")).toDouble();
		return stats;
	}

	return std::nullopt;
}

bool Database::isPlaylistExist(int32_t playlist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);

	IfFailureThrow1(query);
	return query.next();
}

int32_t Database::findTablePlaylistId(int32_t table_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT playlistId FROM tablePlaylist WHERE tableId = (:tableId)"));
	query.bindValue(Q_TEXT(":tableId"), table_id);

	IfFailureThrow1(query);
	while (query.next()) {
		return query.value(Q_TEXT("playlistId")).toInt();
	}
	return kInvalidId;
}

void Database::addTablePlaylist(int32_t table_id, int32_t playlist_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT INTO tablePlaylist (playlistId, tableId) VALUES (:playlistId, :tableId)
    )"));

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	query.bindValue(Q_TEXT(":tableId"), table_id);

	IfFailureThrow1(query);
}

QString Database::getArtistCoverId(int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

QString Database::getAlbumCoverId(int32_t album_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM albums WHERE albumId = (:albumId)"));
	query.bindValue(Q_TEXT(":albumId"), album_id);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

QString Database::getAlbumCoverId(const QString& album) const {
	QSqlQuery query;

	query.prepare(Q_TEXT("SELECT coverId FROM albums WHERE album = (:album)"));
	query.bindValue(Q_TEXT(":album"), album);

	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("coverId"));
	if (query.next()) {
		return query.value(index).toString();
	}
	return Qt::EmptyString;
}

size_t Database::getParentPathHash(const QString & parent_path) const {
	QSqlQuery query;
	query.prepare(Q_TEXT(R"(
	SELECT
		parentPathHash 
	FROM
		musics 
	WHERE
		parentPath = :parentPath
	ORDER BY
		ROWID ASC 
	LIMIT 1
    )")
	);

	query.bindValue(Q_TEXT(":parentPath"), parent_path);
	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("parentPathHash"));
	if (query.next()) {
		// SQLite不支援uint64_t格式但可以使用QByteArray保存.
		auto blob_size_t = query.value(index).toByteArray();
		size_t result = 0;
		memcpy(&result, blob_size_t.data(), blob_size_t.length());
		return result;
	}
	return 0;
}

int32_t Database::addOrUpdateMusic(const TrackInfo& track_info) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitrate, samplerate, offset, dateTime, album_replay_gain, track_replay_gain, album_peak, track_peak, genre, comment, year, fileSize, parentPathHash)
    VALUES ((SELECT musicId FROM musics WHERE path = :path and offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitrate, :samplerate, :offset, :dateTime, :album_replay_gain, :track_replay_gain, :album_peak, :track_peak, :genre, :comment, :year, :fileSize, :parentPathHash)
    )")
	);

	query.bindValue(Q_TEXT(":title"), QString::fromStdWString(track_info.title));
	query.bindValue(Q_TEXT(":track"), track_info.track);
	query.bindValue(Q_TEXT(":path"), QString::fromStdWString(track_info.file_path));
	query.bindValue(Q_TEXT(":fileExt"), QString::fromStdWString(track_info.file_ext));
	query.bindValue(Q_TEXT(":fileName"), QString::fromStdWString(track_info.file_name));
	query.bindValue(Q_TEXT(":parentPath"), QString::fromStdWString(track_info.parent_path));
	query.bindValue(Q_TEXT(":duration"), track_info.duration);
	query.bindValue(Q_TEXT(":durationStr"), streamTimeToString(track_info.duration));
	query.bindValue(Q_TEXT(":bitrate"), track_info.bitrate);
	query.bindValue(Q_TEXT(":samplerate"), track_info.samplerate);
	query.bindValue(Q_TEXT(":offset"), track_info.offset);
	query.bindValue(Q_TEXT(":fileSize"), track_info.file_size);

	// SQLite不支援uint64_t格式但可以使用QByteArray保存.
	QByteArray blob_size_t;
	blob_size_t.resize(sizeof(size_t));
	memcpy(blob_size_t.data(), &track_info.parent_path_hash, sizeof(size_t));
	query.bindValue(Q_TEXT(":parentPathHash"), blob_size_t);

	if (track_info.replay_gain) {
		query.bindValue(Q_TEXT(":album_replay_gain"), track_info.replay_gain.value().album_gain);
		query.bindValue(Q_TEXT(":track_replay_gain"), track_info.replay_gain.value().track_gain);
		query.bindValue(Q_TEXT(":album_peak"), track_info.replay_gain.value().album_peak);
		query.bindValue(Q_TEXT(":track_peak"), track_info.replay_gain.value().track_peak);
	}
	else {
		query.bindValue(Q_TEXT(":album_replay_gain"), 0);
		query.bindValue(Q_TEXT(":track_replay_gain"), 0);
		query.bindValue(Q_TEXT(":album_peak"), 0);
		query.bindValue(Q_TEXT(":track_peak"), 0);
	}

	if (track_info.last_write_time == 0) {
		query.bindValue(Q_TEXT(":dateTime"), QDateTime::currentSecsSinceEpoch());
	}
	else {
		query.bindValue(Q_TEXT(":dateTime"), track_info.last_write_time);
	}

	query.bindValue(Q_TEXT(":genre"), QString::fromStdWString(track_info.genre));
	query.bindValue(Q_TEXT(":comment"), QString::fromStdWString(track_info.comment));
	query.bindValue(Q_TEXT(":year"), track_info.year);

	if (!query.exec()) {
		XAMP_LOG_D(logger_, "{}", query.lastError().text().toStdString());
		return kInvalidId;
	}

	const auto music_id = query.lastInsertId().toInt();

	XAMP_LOG_D(logger_, "addOrUpdateMusic musicId:{}", music_id);

	return music_id;
}

void Database::updateMusicFilePath(int32_t music_id, const QString& file_path) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":path"), file_path);

	query.exec();
}

void Database::updateMusicRating(int32_t music_id, int32_t rating) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":rating"), rating);

	IfFailureThrow1(query);
}

void Database::updateMusicTitle(int32_t music_id, const QString& title) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET title = :title WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":title"), title);

	IfFailureThrow1(query);
}

void Database::updateReplayGain(int32_t music_id,
	double album_rg_gain,
	double album_peak,
	double track_rg_gain,
	double track_peak) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE musics SET album_replay_gain = :album_replay_gain, album_peak = :album_peak, track_replay_gain = :track_replay_gain, track_peak = :track_peak WHERE (musicId = :musicId)"));

	query.bindValue(Q_TEXT(":musicId"), music_id);
	query.bindValue(Q_TEXT(":album_replay_gain"), album_rg_gain);
	query.bindValue(Q_TEXT(":album_peak"), album_peak);
	query.bindValue(Q_TEXT(":track_replay_gain"), track_rg_gain);
	query.bindValue(Q_TEXT(":track_peak"), track_peak);

	IfFailureThrow1(query);
}

void Database::addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const {
	QSqlQuery query;

	const auto querystr = Q_STR("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId) VALUES (NULL, %1, %2, %3)")
		.arg(playlist_id)
		.arg(music_id)
		.arg(album_id);

	query.prepare(querystr);
	IfFailureThrow1(query);
}


void Database::addMusicToPlaylist(const Vector<int32_t>& music_id, int32_t playlist_id) const {
	QSqlQuery query;

	QStringList strings;
	strings.reserve(music_id.size());

	for (const auto id : music_id) {
		strings << Q_TEXT("(") + Q_TEXT("NULL, ") + QString::number(playlist_id) + Q_TEXT(", ") + QString::number(id) + Q_TEXT(")");
	}

	const auto querystr = Q_TEXT("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES ")
	+ strings.join(Q_TEXT(","));
	query.prepare(querystr);
	IfFailureThrow1(query);
}

void Database::updateArtistCoverId(int32_t artist_id, const QString& cover_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":coverId"), cover_id);

	IfFailureThrow1(query);
}

int32_t Database::addOrUpdateArtist(const QString& artist) {
	XAMP_ASSERT(!artist.isEmpty());

	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"));

	auto firstChar = artist.left(1);
	query.bindValue(Q_TEXT(":artist"), artist);
	query.bindValue(Q_TEXT(":firstChar"), firstChar.toUpper());

	IfFailureThrow1(query);

	const auto artist_id = query.lastInsertId().toInt();
	return artist_id;
}

void Database::updateArtistByDiscId(const QString& disc_id, const QString& artist) {
	XAMP_ASSERT(!artist.isEmpty());

	QSqlQuery query;
	
	query.prepare(Q_TEXT(R"(
    SELECT
		albumMusic.artistId
	FROM
		albumMusic
	JOIN albums ON albums.albumId = albumMusic.albumId 
	WHERE
		albums.discId = :discId
	LIMIT 1	
    )"));

	query.bindValue(Q_TEXT(":discId"), disc_id);
	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("artistId"));
	if (query.next()) {
		auto artist_id = query.value(index).toInt();
		query.prepare(Q_TEXT("UPDATE artists SET artist = :artist WHERE (artistId = :artistId)"));
		query.bindValue(Q_TEXT(":artistId"), artist_id);
		query.bindValue(Q_TEXT(":artist"), artist);
		IfFailureThrow1(query);
	}
}

int32_t Database::getAlbumIdByDiscId(const QString& disc_id) const {
	QSqlQuery query;
	query.prepare(Q_TEXT("SELECT albumId FROM albums WHERE discId = (:discId)"));
	query.bindValue(Q_TEXT(":discId"), disc_id);
	IfFailureThrow1(query);

	const auto index = query.record().indexOf(Q_TEXT("albumId"));
	if (query.next()) {
		return query.value(index).toInt();
	}
	return kInvalidId;
}

void Database::updateAlbumByDiscId(const QString& disc_id, const QString& album) {
	QSqlQuery query;

	query.prepare(Q_TEXT("UPDATE albums SET album = :album WHERE (discId = :discId)"));

	query.bindValue(Q_TEXT(":album"), album);
	query.bindValue(Q_TEXT(":discId"), disc_id);

	IfFailureThrow1(query);
}

int32_t Database::addOrUpdateAlbum(const QString& album, int32_t artist_id, int64_t album_time, bool is_podcast, const QString& disc_id) {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albums (albumId, album, artistId, firstChar, coverId, isPodcast, dateTime, discId)
    VALUES ((SELECT albumId FROM albums WHERE album = :album), :album, :artistId, :firstChar, :coverId, :isPodcast, :dateTime, :discId)
    )"));

	const auto first_char = album.left(1);

	query.bindValue(Q_TEXT(":album"), album);
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":firstChar"), first_char.toUpper());
	query.bindValue(Q_TEXT(":coverId"), getAlbumCoverId(album));
	query.bindValue(Q_TEXT(":isPodcast"), is_podcast ? 1 : 0);
	query.bindValue(Q_TEXT(":dateTime"), album_time);
	query.bindValue(Q_TEXT(":discId"), disc_id);

	IfFailureThrow1(query);

	const auto album_id = query.lastInsertId().toInt();

	XAMP_LOG_D(logger_, "addOrUpdateAlbum albumId:{}", album_id);

	return album_id;
}

void Database::addOrUpdateAlbumArtist(int32_t album_id, int32_t artist_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albumArtist (albumArtistId, albumId, artistId)
    VALUES ((SELECT albumArtistId from albumArtist where albumId = :albumId AND artistId = :artistId), :albumId, :artistId)
    )"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":artistId"), artist_id);

	IfFailureThrow1(query);

	XAMP_LOG_D(logger_, "addOrUpdateAlbumArtist albumId:{} artistId:{}", album_id, artist_id);
}

void Database::addOrUpdateAlbumMusic(int32_t album_id, int32_t artist_id, int32_t music_id) const {
	QSqlQuery query;

	query.prepare(Q_TEXT(R"(
    INSERT OR REPLACE INTO albumMusic (albumMusicId, albumId, artistId, musicId)
    VALUES ((SELECT albumMusicId from albumMusic where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId)
    )"));

	query.bindValue(Q_TEXT(":albumId"), album_id);
	query.bindValue(Q_TEXT(":artistId"), artist_id);
	query.bindValue(Q_TEXT(":musicId"), music_id);

	IfFailureThrow1(query);

	addOrUpdateAlbumArtist(album_id, artist_id);
}

void Database::removePlaylistAllMusic(int32_t playlist_id) {
	QSqlQuery query;
	query.prepare(Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId"));
	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
	XAMP_LOG_D(logger_, "removePlaylistAllMusic playlist_id:{}", playlist_id);
}

void Database::removeMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
	QSqlQuery query;

	QString str = Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)");

	QStringList list;
	for (auto id : select_music_ids) {
		list << QString::number(id);
	}

	auto q = str.arg(list.join(Q_TEXT(",")));
	query.prepare(q);

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

void Database::removePlaylistMusic(int32_t playlist_id, const QVector<int32_t>& select_music_ids) {
	QSqlQuery query;

	QString str = Q_TEXT("DELETE FROM playlistMusics WHERE playlistId=:playlistId AND playlistMusicsId in (%0)");

	QStringList list;
	for (auto id : select_music_ids) {
		list << QString::number(id);
	}

	auto q = str.arg(list.join(Q_TEXT(",")));
	query.prepare(q);

	query.bindValue(Q_TEXT(":playlistId"), playlist_id);
	IfFailureThrow1(query);
}

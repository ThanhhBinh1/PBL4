import 'dart:async';
import 'package:sqflite/sqflite.dart';
import 'package:path/path.dart';

class HistoryItem {
  final int? id;
  final String word;
  final String meaning;
  final String ipa;
  final String imagePath;
  final String timestamp;

  HistoryItem({
    this.id, 
    required this.word, 
    required this.meaning, 
    required this.ipa, 
    required this.imagePath, 
    required this.timestamp
  });

  Map<String, dynamic> toMap() {
    return {
      'word': word, 
      'meaning': meaning, 
      'ipa': ipa, 
      'imagePath': imagePath, 
      'timestamp': timestamp
    };
  }
}

class DatabaseHelper {
  static final DatabaseHelper instance = DatabaseHelper._init();
  static Database? _database;
  DatabaseHelper._init();

  Future<Database> get database async {
    if (_database != null) return _database!;
    _database = await _initDB('history.db');
    return _database!;
  }

  Future<Database> _initDB(String filePath) async {
    final dbPath = await getDatabasesPath();
    final path = join(dbPath, filePath);
    return await openDatabase(path, version: 1, onCreate: (db, version) {
      return db.execute(
          'CREATE TABLE history(id INTEGER PRIMARY KEY AUTOINCREMENT, word TEXT, meaning TEXT, ipa TEXT, imagePath TEXT, timestamp TEXT)');
    });
  }

  Future<int> create(HistoryItem item) async {
    final db = await instance.database;
    return await db.insert('history', item.toMap());
  }

  Future<List<HistoryItem>> readAllNotes() async {
    final db = await instance.database;
    final result = await db.query('history', orderBy: 'id DESC');
    return result.map((json) => HistoryItem(
      id: json['id'] as int,
      word: json['word'] as String,
      meaning: json['meaning'] as String,
      ipa: json['ipa'] as String,
      imagePath: json['imagePath'] as String,
      timestamp: json['timestamp'] as String,
    )).toList();
  }
}
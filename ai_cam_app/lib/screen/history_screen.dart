import 'dart:io';
import 'package:flutter/material.dart';
import '../db/database_helper.dart';

class HistoryScreen extends StatefulWidget {
  const HistoryScreen({super.key});
  @override
  State<HistoryScreen> createState() => _HistoryScreenState();
}

class _HistoryScreenState extends State<HistoryScreen> {
  late Future<List<HistoryItem>> _historyList;

  @override
  void initState() {
    super.initState();
    _historyList = DatabaseHelper.instance.readAllNotes();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("History")),
      body: FutureBuilder<List<HistoryItem>>(
        future: _historyList,
        builder: (context, snapshot) {
          if (!snapshot.hasData) return const Center(child: CircularProgressIndicator());
          if (snapshot.data!.isEmpty) return const Center(child: Text("Chưa có dữ liệu"));

          return ListView.builder(
            itemCount: snapshot.data!.length,
            itemBuilder: (context, index) {
              final item = snapshot.data![index];
              return Card(
                margin: const EdgeInsets.symmetric(horizontal: 10, vertical: 5),
                child: ListTile(
                  leading: Container(
                    width: 60, height: 60,
                    decoration: BoxDecoration(borderRadius: BorderRadius.circular(8)),
                    child: Image.file(File(item.imagePath), fit: BoxFit.cover, 
                      errorBuilder: (c,o,s) => const Icon(Icons.broken_image)),
                  ),
                  title: Text(item.word.toUpperCase(), style: const TextStyle(fontWeight: FontWeight.bold)),
                  subtitle: Text("${item.ipa}\n${item.meaning}"),
                  isThreeLine: true,
                  trailing: Text(item.timestamp.split(' ')[1], style: const TextStyle(color: Colors.grey)),
                ),
              );
            },
          );
        },
      ),
    );
  }
}
import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:path_provider/path_provider.dart';
import 'package:intl/intl.dart';

import '../config/config.dart';
import '../database_helper.dart';
import 'stream_screen.dart';
import 'history_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  Timer? _timer;
  int lastBatchId = -1;
  bool isScanning = true;
  
  // Dữ liệu hiển thị
  String currentWord = "---";
  String currentMeaning = "---";
  String currentIPA = "---";
  File? currentImageFile;

  @override
  void initState() {
    super.initState();
    startPolling();
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  void startPolling() {
    _timer = Timer.periodic(const Duration(milliseconds: 500), (timer) {
      checkServerInfo();
    });
  }

  // 1. Hỏi Server
  Future<void> checkServerInfo() async {
    try {
      final response = await http.get(Uri.parse('$SERVER_IP/mobile/info')).timeout(const Duration(seconds: 2));
      if (response.statusCode == 200) {
        final json = jsonDecode(response.body);
        int batchId = json['batch_id'];
        String object = json['object'];
        bool hasImage = json['has_image'];

        if (batchId != lastBatchId) {
          lastBatchId = batchId;
          if (object != "none") {
            processNewDetection(object, hasImage);
          } else {
            setState(() => isScanning = true);
          }
        }
      }
    } catch (e) {
       // Silent error to avoid console spam
    }
  }

  // 2. Xử lý vật thể mới
  Future<void> processNewDetection(String word, bool hasImage) async {
    setState(() => isScanning = false);
    
    // Tải ảnh
    File? imgFile;
    if (hasImage) {
      imgFile = await downloadImage(word);
    }

    // Lấy nghĩa và IPA
    String meaning = await translateWord(word);
    String ipa = await fetchIPA(word);

    setState(() {
      currentWord = word;
      currentMeaning = meaning;
      currentIPA = ipa;
      currentImageFile = imgFile;
    });

    // Lưu lịch sử
    if (imgFile != null) {
      await DatabaseHelper.instance.create(HistoryItem(
        word: word,
        meaning: meaning,
        ipa: ipa,
        imagePath: imgFile.path,
        timestamp: DateFormat('yyyy-MM-dd HH:mm:ss').format(DateTime.now()),
      ));
    }
  }

  Future<File?> downloadImage(String word) async {
    try {
      final response = await http.get(Uri.parse('$SERVER_IP/mobile/image'));
      if (response.statusCode == 200) {
        final dir = await getApplicationDocumentsDirectory();
        final file = File('${dir.path}/${word}_${DateTime.now().millisecondsSinceEpoch}.jpg');
        await file.writeAsBytes(response.bodyBytes);
        return file;
      }
    } catch (e) { print(e); }
    return null;
  }

  Future<String> translateWord(String word) async {
    try {
      final url = Uri.parse('https://api.mymemory.translated.net/get?q=$word&langpair=en|vi');
      final response = await http.get(url);
      if (response.statusCode == 200) {
        final json = jsonDecode(response.body);
        return json['responseData']['translatedText'] ?? "Unknown";
      }
    } catch (e) { return "Error"; }
    return "---";
  }

  Future<String> fetchIPA(String word) async {
    try {
      final url = Uri.parse('https://api.dictionaryapi.dev/api/v2/entries/en/$word');
      final response = await http.get(url);
      if (response.statusCode == 200) {
        final List json = jsonDecode(response.body);
        if (json.isNotEmpty) {
           var phonetics = json[0]['phonetics'] as List;
           for(var p in phonetics) {
             if(p['text'] != null) return p['text'];
           }
        }
      }
    } catch (e) { return "/.../"; }
    return "/.../";
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('AI Learning'), backgroundColor: Colors.blueAccent, foregroundColor: Colors.white),
      drawer: Drawer(
        child: ListView(
          padding: EdgeInsets.zero,
          children: [
            const DrawerHeader(
              decoration: BoxDecoration(color: Colors.blue),
              child: Text('Menu', style: TextStyle(color: Colors.white, fontSize: 24)),
            ),
            ListTile(
              leading: const Icon(Icons.home),
              title: const Text('Home'),
              onTap: () => Navigator.pop(context),
            ),
            ListTile(
              leading: const Icon(Icons.videocam),
              title: const Text('Live Stream'),
              onTap: () {
                Navigator.pop(context);
                Navigator.push(context, MaterialPageRoute(builder: (_) => const StreamScreen()));
              },
            ),
            ListTile(
              leading: const Icon(Icons.history),
              title: const Text('History'),
              onTap: () {
                Navigator.pop(context);
                Navigator.push(context, MaterialPageRoute(builder: (_) => const HistoryScreen()));
              },
            ),
          ],
        ),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          children: [
            Expanded(
              flex: 5,
              child: Container(
                width: double.infinity,
                decoration: BoxDecoration(
                  color: Colors.black12,
                  borderRadius: BorderRadius.circular(12),
                  border: Border.all(color: Colors.blueAccent, width: 2)
                ),
                child: currentImageFile != null
                    ? ClipRRect(
                        borderRadius: BorderRadius.circular(10),
                        child: Image.file(currentImageFile!, fit: BoxFit.cover),
                      )
                    : Center(
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            const Icon(Icons.camera_alt, size: 50, color: Colors.grey),
                            Text(isScanning ? "Scanning..." : "Waiting...", style: const TextStyle(color: Colors.grey)),
                          ],
                        ),
                      ),
              ),
            ),
            const SizedBox(height: 20),
            Expanded(
              flex: 4,
              child: Card(
                elevation: 5,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                child: Padding(
                  padding: const EdgeInsets.all(20.0),
                  child: Container(
                    width: double.infinity,
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Text(currentWord.toUpperCase(), style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold, color: Colors.blue)),
                        const SizedBox(height: 10),
                        Text(currentIPA, style: const TextStyle(fontSize: 20, color: Colors.orange, fontStyle: FontStyle.italic)),
                        const Divider(height: 30, thickness: 1),
                        Text(currentMeaning, style: const TextStyle(fontSize: 26, color: Colors.green, fontWeight: FontWeight.w500)),
                      ],
                    ),
                  ),
                ),
              ),
            )
          ],
        ),
      ),
    );
  }
}
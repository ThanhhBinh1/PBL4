import 'package:flutter/material.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';
import '../config/config.dart';

class StreamScreen extends StatelessWidget {
  const StreamScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Live Stream")),
      body: Center(
        child: Mjpeg(
          isLive: true,
          stream: '$SERVER_IP/video_feed', // Endpoint stream
          error: (context, error, stack) => Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const Icon(Icons.error, color: Colors.red, size: 50),
              const SizedBox(height: 10),
              Text("Lỗi kết nối Stream:\n$error", textAlign: TextAlign.center),
            ],
          ),
        ),
      ),
    );
  }
}
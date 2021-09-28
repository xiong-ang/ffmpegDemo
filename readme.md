# video capture
> 从文件或rtsp流中获取帧数据  
> 参考： 源码实例：test/api/api-h264-test.c  
> 官方实例： https://ffmpeg.org/doxygen/trunk/examples.html  

# video_recoder
> 从rtsp获取视频流并保存文件，支持H.264和H.265保存mp4格式  
> 参考: https://blog.csdn.net/weixin_42432281/article/details/95512294  
> H256可行方案：https://www.cnblogs.com/lifan3a/articles/7325912.html  

# 注意点
> 摄像头视频配置中，不仅仅视频编码选项，视频类型选项也会有影响，如果是混合流，会导致取到错误的流  
> 确实如网上所说，如果保存的mp4文件读取不了，可能是H265比H264需要更多的codec配置选项  

## 参考资料
> ffmpeg教程：http://dranger.com/ffmpeg/tutorial01.html  
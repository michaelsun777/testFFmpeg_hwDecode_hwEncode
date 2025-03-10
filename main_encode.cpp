extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

#include <iostream>


// 帧率（30帧/秒）
const AVRational frame_rate = { 30, 1 };


// 输出文件名
const char* output_filename = "encode_output.mp4";


AVBufferRef* hw_device_ctx = nullptr; // 硬件设备上下文
AVCodecContext* codec_ctx = nullptr; // 编码器上下文

// FFmpeg 相关上下文和结构体
AVFormatContext* fmt_ctx = nullptr;  // 输出文件上下文
AVFrame* hw_frame = nullptr;         // 硬件帧
AVFrame* sw_frame = nullptr;         // 软件帧
AVPacket* pkt = nullptr;             // 编码后的数据包


// 视频分辨率
const int width = 1280;
const int height = 534;



int main(int argc, char *argv[])
{

    enum AVHWDeviceType type;
    // 设备类型为：cuda dxva2 qsv d3d11va opencl，通常在windows使用d3d11va或者dxva2
	type = av_hwdevice_find_type_by_name("vaapi"); // 根据设备名找到设备类型
    if (type == AV_HWDEVICE_TYPE_NONE)
	{
		//fprintf(stderr, "Device type %s is not supported.\n", argv[1]);
		fprintf(stderr, "Available device types:");
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
			fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
		fprintf(stderr, "\n");
		return -1;
	}

    // 1. 初始化硬件设备上下文 // 硬件加速初始化
    int ret = av_hwdevice_ctx_create(&hw_device_ctx, type, nullptr, nullptr, 0);
    if (ret < 0)
    {
        throw std::runtime_error("Failed to create hardware device context");
    }

    // 2. 创建硬件帧上下文
    AVBufferRef *hw_frames_ref = av_hwframe_ctx_alloc(hw_device_ctx);
    if (!hw_frames_ref)
    {
        throw std::runtime_error("Failed to create hardware frames context");
    }

    // 配置硬件帧上下文参数
    AVHWFramesContext *hw_frames_ctx = (AVHWFramesContext *)hw_frames_ref->data;
    hw_frames_ctx->format = AV_PIX_FMT_VAAPI;     // 硬件像素格式
    hw_frames_ctx->sw_format = AV_PIX_FMT_NV12; // 软件像素格式
    hw_frames_ctx->width = width;               // 视频宽度
    hw_frames_ctx->height = height;             // 视频高度
    hw_frames_ctx->initial_pool_size = 20;      // 初始帧池大小

    // 初始化硬件帧上下文
    ret = av_hwframe_ctx_init(hw_frames_ref);
    if (ret < 0)
    {
        throw std::runtime_error("Failed to initialize hardware frames context");
    }

    // 3. 查找编码器（使用 hevc_vaapi 编码器）
    const AVCodec *codec = avcodec_find_encoder_by_name("hevc_vaapi");
    if (!codec)
    {
        throw std::runtime_error("Codec vaapi not found");
    }

    // 4. 创建编码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        throw std::runtime_error("Could not allocate video codec context");
    }

    // 配置编码器参数
    codec_ctx->hw_frames_ctx = av_buffer_ref(hw_frames_ref); // 绑定硬件帧上下文
    codec_ctx->width = width;                                // 视频宽度
    codec_ctx->height = height;                              // 视频高度
    codec_ctx->time_base = av_inv_q(frame_rate);             // 时间基（帧率的倒数）
    codec_ctx->framerate = frame_rate;                       // 帧率
    codec_ctx->pix_fmt = AV_PIX_FMT_VAAPI;                     // 像素格式
    codec_ctx->bit_rate = 4000000;                           // 码率（4 Mbps）
    codec_ctx->gop_size = 1;                                 // GOP 大小（关键帧间隔）

    // 打开编码器
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0)
    {
        throw std::runtime_error("Could not open codec");
    }

    // 5. 创建硬件帧
    hw_frame = av_frame_alloc();
    if (!hw_frame)
    {
        throw std::runtime_error("Could not allocate video frame");
    }
    hw_frame->format = AV_PIX_FMT_VAAPI; // 硬件像素格式
    hw_frame->width = width;           // 视频宽度
    hw_frame->height = height;         // 视频高度

    // 为硬件帧分配内存
    ret = av_hwframe_get_buffer(av_buffer_ref(hw_frames_ref), hw_frame, 0);
    if (ret < 0)
    {
        throw std::runtime_error("Could not allocate hardware frame buffer");
    }

    // 6. 创建软件帧
    sw_frame = av_frame_alloc();
    if (!sw_frame)
    {
        throw std::runtime_error("Could not allocate software frame");
    }
    sw_frame->format = AV_PIX_FMT_NV12; // 软件像素格式
    sw_frame->width = width;            // 视频宽度
    sw_frame->height = height;          // 视频高度

    // 为软件帧分配内存
    ret = av_frame_get_buffer(sw_frame, 0);
    if (ret < 0)
    {
        throw std::runtime_error("Could not allocate software frame buffer");
    }

    // 7. 创建输出文件上下文
    ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, nullptr, output_filename);
    if (ret < 0)
    {
        throw std::runtime_error("Could not create output context");
    }

    // 8. 创建视频流
    AVStream *stream = avformat_new_stream(fmt_ctx, nullptr);
    if (!stream)
    {
        throw std::runtime_error("Could not create video stream");
    }

    // 从编码器上下文复制参数到视频流
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    stream->time_base = AVRational{1, 90000}; // 时间基

    // 9. 打开输出文件
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&fmt_ctx->pb, output_filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            throw std::runtime_error("Could not open output file");
        }
    }

    // 10. 写入文件头
    ret = avformat_write_header(fmt_ctx, nullptr);
    if (ret < 0)
    {
        throw std::runtime_error("Error writing header to output file");
    }

    // 11. 编码帧
    FILE *f = fopen("output.nv12", "rb"); // 打开 NV12 文件
    if (!f)
    {
        throw std::runtime_error("Could not open YUV file");
    }

    int read_size = width * height * 15 / 10;//(15 / 10) = 1.5f

    for (int i = 0; i < 100; i++)
    { // 编码 50 帧
        // 从 NV12 文件读取数据到软件帧        
        size_t redasize = fread(sw_frame->data[0], 1, read_size, f);     // YUV  yyyy yyyy uv uv 分量
        //fread(sw_frame->data[0], 1, width * height, f);     // Y 分量
        //fread(sw_frame->data[1], 1, width * height / 2, f); // UV 分量
        if (redasize != read_size)
        {
            break;
        }

        // 将软件帧数据拷贝到硬件帧
        ret = av_hwframe_transfer_data(hw_frame, sw_frame, 0);
        if (ret < 0)
        {
            throw std::runtime_error("Error transferring data to hardware frame");
        }

        // 设置帧的显示时间戳（PTS）
        hw_frame->pts = i * 3000;                   // PTS = 帧序号 * 帧间隔
        //hw_frame->time_base = AVRational{1, 90000}; // 时间基

        // 发送帧到编码器
        ret = avcodec_send_frame(codec_ctx, hw_frame);
        if (ret < 0)
        {
            throw std::runtime_error("Error sending frame to encoder");
        }

        // 接收编码后的数据包
        pkt = av_packet_alloc();
        while (ret >= 0)
        {
            ret = avcodec_receive_packet(codec_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                av_packet_free(&pkt);
                break;
            }

            // 设置数据包的流索引和时间基
            pkt->stream_index = stream->index;
            //pkt->time_base = AVRational{1, 90000};

            // 写入数据包到输出文件
            ret = av_interleaved_write_frame(fmt_ctx, pkt);
            if (ret < 0)
            {
                throw std::runtime_error("Error writing packet to file");
            }

            // 释放数据包
            av_packet_unref(pkt);
        }
    }

    // 12. 刷新编码器（发送空帧以刷新缓冲区）
    ret = avcodec_send_frame(codec_ctx, nullptr);
    while (ret >= 0)
    {
        pkt = av_packet_alloc();
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR_EOF)
        {
            break;
        }

        // 写入剩余的数据包到输出文件
        pkt->stream_index = stream->index;
        //pkt->time_base = AVRational{1, 90000};
        ret = av_interleaved_write_frame(fmt_ctx, pkt);
        av_packet_unref(pkt);
    }

    // 13. 写入文件尾
    av_write_trailer(fmt_ctx);

    // 关闭 YUV 文件
    fclose(f);

    std::cout << "Encoding completed successfully!" << std::endl;

    // 计算并输出编码过程的总耗时
    // std::cout << "Total time taken: "
    //           << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - nows).count()
    //           << " milliseconds" << std::endl;

    // 14. 释放资源
    if (fmt_ctx && !(fmt_ctx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    av_frame_free(&hw_frame);
    av_frame_free(&sw_frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codec_ctx);
    av_buffer_unref(&hw_device_ctx);

    return 0;
}
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/point_cloud2.hpp"
#include "sensor_msgs/point_cloud2_iterator.hpp"

class PointCloudIntensityNode : public rclcpp::Node
{
public:
  PointCloudIntensityNode() : Node("pointcloud_intensity_node")
  {
    pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("registered_scan", 10);
    sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "/points", 10,
      std::bind(&PointCloudIntensityNode::pointCloudCallback, this, std::placeholders::_1));
  }

private:
  void pointCloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg)
  {
    // 复制原点云
    auto output = std::make_shared<sensor_msgs::msg::PointCloud2>();
    *output = *msg;

    // 检查是否已经有intensity字段，没有就添加
    bool has_intensity = false;
    for (auto & field : output->fields) {
      if (field.name == "intensity") {
        has_intensity = true;
        break;
      }
    }

    if (!has_intensity) {
      // 添加intensity字段
      sensor_msgs::msg::PointField intensity_field;
      intensity_field.name = "intensity";
      intensity_field.offset = output->point_step; // 在点结构后面追加
      intensity_field.datatype = sensor_msgs::msg::PointField::FLOAT32;
      intensity_field.count = 1;

      output->fields.push_back(intensity_field);
      output->point_step += 4; // 4 bytes for float32

      // 重新分配data buffer
      std::vector<uint8_t> new_data;
      new_data.resize(output->height * output->width * output->point_step);

      // 复制每个点，附加intensity=1.0
      for (size_t i = 0; i < output->height * output->width; i++) {
        size_t old_pos = i * (output->point_step - 4);
        size_t new_pos = i * output->point_step;
        memcpy(&new_data[new_pos], &output->data[old_pos], output->point_step - 4);
        float intensity_val = 1.0f;
        memcpy(&new_data[new_pos + output->point_step - 4], &intensity_val, sizeof(float));
      }

      output->data = std::move(new_data);
    }

    pub_->publish(*output);
  }

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PointCloudIntensityNode>());
  rclcpp::shutdown();
  return 0;
}

package require -exact qsys 18.0
create_system nv_flash
set_project_property DEVICE_FAMILY {MAX 10}
set_project_property DEVICE 10M50DAF484C7G
add_instance f altera_onchip_flash
set_instance_parameter_value f CONFIGURATION_MODE {Single Compressed Image with Memory Initialization}
set_instance_parameter_value f DATA_INTERFACE {Parallel}
set_instance_parameter_value f READ_BURST_MODE {Incrementing}
set_instance_parameter_value f READ_BURST_COUNT {2}
set_instance_parameter_value f CLOCK_FREQUENCY {50.0}
set_instance_parameter_value f SECTOR_ACCESS_MODE {{Read and write} {Read and write} Hidden Hidden Hidden}
add_interface clk clock sink
set_interface_property clk EXPORT_OF f.clk
add_interface nreset reset sink
set_interface_property nreset EXPORT_OF f.nreset
add_interface data avalon slave
set_interface_property data EXPORT_OF f.data
add_interface csr avalon slave
set_interface_property csr EXPORT_OF f.csr
save_system nv_flash.qsys

include $(TOPDIR)/rules.mk

PKG_NAME:=cpp-demo
PKG_RELEASE:=1
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/cpp-demo
	SECTION:=utils
	CATEGORY:=Utilties
	TITLE:=cpp-demo
	DEPENDS:=+libstdcpp
endef

define Build/Prepare
	$(MKDIR) -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)
endef

define Package/cpp-demo/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cpp-demo $(1)/bin
endef

$(eval $(call BuildPackage,cpp-demo))

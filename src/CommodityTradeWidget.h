#ifndef _COMMODITYTRADEWIDGET_H
#define _COMMODITYTRADEWIDGET_H

#include "Gui.h"
#include "MarketAgent.h"
#include <map>

class CommodityTradeWidget : public Gui::Fixed {
public:
	CommodityTradeWidget(MarketAgent *seller);
	void ShowAll();
	sigc::signal<void,int> onClickSell;
	sigc::signal<void,int> onClickBuy;
private:
	void OnClickBuy(int commodity_type) {
		onClickBuy.emit(commodity_type);
		UpdateStock(commodity_type);
	}
	void OnClickSell(int commodity_type) {
		onClickSell.emit(commodity_type);
		UpdateStock(commodity_type);
	}
	void UpdateStock(int commodity_type);

	std::map<int, Gui::Label*> m_stockLabels;
	std::map<int, Gui::Label*> m_cargoLabels;
	MarketAgent *m_seller;
};

#endif /* _COMMODITYTRADEWIDGET_H */

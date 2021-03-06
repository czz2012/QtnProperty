/*
   Copyright (c) 2012-2016 Alex Zhondin <lexxmark.dev@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "PropertyDelegateLayer.h"
#include "QtnProperty/Delegates/PropertyDelegateFactory.h"
#include "PropertyLayer.h"
#include "QtnProperty/Delegates/Utils/PropertyEditorHandler.h"

#include <QComboBox>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QPaintEvent>

void regLayerDelegates()
{
	QtnPropertyDelegateFactory::staticInstance().registerDelegateDefault(
		&QtnPropertyLayerBase::staticMetaObject,
		&qtnCreateDelegate<QtnPropertyDelegateLayer, QtnPropertyLayerBase>);
}

static void drawLayer(
	QStyle *style, const LayerInfo &layer, QPainter &painter, const QRect &rect)
{
	QRect textRect = rect;

	QRect colorRect = rect;
	colorRect.setWidth(colorRect.height());
	colorRect.adjust(2, 2, -2, -2);

	painter.fillRect(colorRect, Qt::black);
	colorRect.adjust(1, 1, -1, -1);
	painter.fillRect(colorRect, layer.color);

	textRect.setLeft(colorRect.right() + 5);

	if (textRect.isValid())
	{
		qtnDrawValueText(layer.name, painter, textRect, style);
	}
}

class QtnPropertyLayerComboBox : public QComboBox
{
public:
	explicit QtnPropertyLayerComboBox(
		QtnPropertyDelegateLayer *delegate, QWidget *parent = Q_NULLPTR)
		: QComboBox(parent)
		, m_delegate(delegate)
		, m_property(static_cast<QtnPropertyLayerBase *>(delegate->property()))
		, m_updating(0)
	{
		m_layers = m_property->layers();
		setLineEdit(nullptr);
		setItemDelegate(new ItemDelegate(m_layers, this));
		for (const auto &layer : m_layers)
		{
			Q_UNUSED(layer);
			addItem(QString());
		}

		updateEditor();

		QObject::connect(m_property, &QtnPropertyBase::propertyDidChange, this,
			&QtnPropertyLayerComboBox::onPropertyDidChange);
		QObject::connect(this,
			static_cast<void (QComboBox::*)(int)>(
				&QComboBox::currentIndexChanged),
			this, &QtnPropertyLayerComboBox::onCurrentIndexChanged);
	}

protected:
	void paintEvent(QPaintEvent *event) override
	{
		QComboBox::paintEvent(event);

		QPainter painter(this);

		auto index = currentIndex();
		if (index != -1)
		{
			Q_ASSERT(index < m_layers.size());
			drawLayer(style(), m_layers[index], painter, event->rect());
		}
	}

	void updateEditor()
	{
		m_updating++;
		setEnabled(m_delegate->stateProperty()->isEditableByUser());
		setCurrentIndex(m_property->value());
		m_updating--;
	}

	void onPropertyDidChange(QtnPropertyChangeReason reason)
	{
		if ((reason & QtnPropertyChangeReasonValue))
			updateEditor();
	}

	void onCurrentIndexChanged(int index)
	{
		if (m_updating)
			return;

		if (index >= 0)
		{
			Q_ASSERT(index < m_layers.size());
			m_property->setValue(index, m_delegate->editReason());
		}
	}

private:
	QtnPropertyDelegateLayer *m_delegate;
	QtnPropertyLayerBase *m_property;
	QList<LayerInfo> m_layers;
	unsigned m_updating;

	class ItemDelegate : public QStyledItemDelegate
	{
	public:
		ItemDelegate(const QList<LayerInfo> &layers, QComboBox *owner)
			: m_layers(layers)
			, m_owner(owner)
		{
		}

		void paint(QPainter *painter, const QStyleOptionViewItem &option,
			const QModelIndex &index) const override
		{
			QStyledItemDelegate::paint(painter, option, index);

			Q_ASSERT(index.row() < m_layers.size());
			drawLayer(
				m_owner->style(), m_layers[index.row()], *painter, option.rect);
		}

	private:
		const QList<LayerInfo> &m_layers;
		QComboBox *m_owner;
	};
};

void QtnPropertyDelegateLayer::drawValueImpl(
	QStylePainter &painter, const QRect &rect) const
{
	auto layer = owner().valueLayer();
	if (layer)
		drawLayer(painter.style(), *layer, painter, rect);
}

QWidget *QtnPropertyDelegateLayer::createValueEditorImpl(
	QWidget *parent, const QRect &rect, QtnInplaceInfo *inplaceInfo)
{
	auto combo = new QtnPropertyLayerComboBox(this, parent);
	combo->setGeometry(rect);
	if (inplaceInfo && stateProperty()->isEditableByUser())
		combo->showPopup();

	return combo;
}

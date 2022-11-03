from django import template

from ..models import Category

register = template.Library()


@register.inclusion_tag('indexer/category_top.djhtml')
def get_category_top():
    categories_likes = Category.objects.order_by('-likes')[:5]
    categories_views = Category.objects.order_by('-views')[:5]
    return {
        'categories_likes': categories_likes,
        'categories_views': categories_views
    }
